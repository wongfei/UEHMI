#include "Audio/HMIAudioProcessor.h"

#include "HMIThirdPartyBegin.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/vad/voice_activity_detector.h"
#include "HMIThirdPartyEnd.h"

DECLARE_CYCLE_STAT(TEXT("WebrtcProcessStream"), STAT_HMI_WebrtcProcessStream, STATGROUP_HMI)

using namespace webrtc;

struct FHMIAudioProcessorImpl
{
	AudioProcessing::Config Config;
	rtc::scoped_refptr<AudioProcessing> AudioProc;
	std::unique_ptr<VoiceActivityDetector> Vad;
};

FHMIAudioProcessor::FHMIAudioProcessor(const FHMIAudioProcessorConfig& InConfig)
{
	CombinedBuf.Reserve(4096);
	OldBuf.Reserve(4096);

	AudioProcessing::Config config;

	// High-pass filter: removes rumble
	config.high_pass_filter.enabled = InConfig.HighPassFilter;

	// Echo cancellation: enable only if not using headphones
	config.echo_canceller.enabled = InConfig.EchoCanceller;
	config.echo_canceller.mobile_mode = InConfig.EchoCancellerMobile; // more aggressive, use only on phone-like devices

	// Noise suppression
	config.noise_suppression.enabled = InConfig.NoiseSuppression;
	config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;

	// Gain Control
	config.gain_controller1.enabled = InConfig.GainController;
	config.gain_controller1.mode = webrtc::AudioProcessing::Config::GainController1::kAdaptiveAnalog;
	config.gain_controller1.analog_gain_controller.enabled = true;
	config.gain_controller1.compression_gain_db = 9;
	config.gain_controller1.enable_limiter = true;

	// Transient suppression (keyboard clicks)
	config.transient_suppression.enabled = InConfig.TransientSuppression;

	auto AudioProc = AudioProcessingBuilder().Create();
	AudioProc->ApplyConfig(config);
	AudioProc->Initialize();

	Impl = MakeUnique<FHMIAudioProcessorImpl>();
	Impl->Config = config;
	Impl->AudioProc = MoveTemp(AudioProc);
	Impl->Vad = std::make_unique<VoiceActivityDetector>();
}

FHMIAudioProcessor::~FHMIAudioProcessor()
{
}

void FHMIAudioProcessor::Flush()
{
	OldBuf.Reset();
}

int FHMIAudioProcessor::Process(TArray<uint8>& PCMData, int NumSamples, int SampleRate, int NumChannels)
{
	if (!Impl)
		return 0;

	SCOPE_CYCLE_COUNTER(STAT_HMI_WebrtcProcessStream)

	StreamConfig input_config(SampleRate, NumChannels);
	StreamConfig output_config(SampleRate, NumChannels);

	LevelControlEnabled = 
		Impl->Config.gain_controller1.enabled && 
		Impl->Config.gain_controller1.mode == AudioProcessing::Config::GainController1::kAdaptiveAnalog &&
		Impl->Config.gain_controller1.analog_gain_controller.enabled;

	if (LevelControlEnabled)
	{
		const float Level = FMath::Clamp(CurrentLevel, 0.0f, 1.0f) * 255.0f;
		Impl->AudioProc->set_stream_analog_level((int)Level);
	}

	const int NumOldBytes = OldBuf.Num();
	const int TotalBytes = NumOldBytes + PCMData.Num();

	CombinedBuf.SetNumUninitialized(TotalBytes, EAllowShrinking::No);

	if (NumOldBytes > 0)
	{
		FMemory::Memcpy(CombinedBuf.GetData(), OldBuf.GetData(), NumOldBytes);
	}

	FMemory::Memcpy(CombinedBuf.GetData() + NumOldBytes, PCMData.GetData(), PCMData.Num());

	// APM processes audio in chunks of about 10 ms. See audio_processing.h GetFrameSize() for details.
	// static int GetFrameSize(int sample_rate_hz) { return sample_rate_hz / 100; }

	const int SamplesPerFrame = Impl->AudioProc->GetFrameSize(SampleRate);
	const int BytesPerFrame = SamplesPerFrame * sizeof(int16);

	const int NFrames = TotalBytes / (BytesPerFrame * NumChannels);
	const int NRem = TotalBytes % (BytesPerFrame * NumChannels);
	int Pos = 0;

	PCMData.SetNumUninitialized(NFrames * (BytesPerFrame * NumChannels), EAllowShrinking::No);

	for (int Id = 0; Id < NFrames; ++Id)
	{
		Impl->AudioProc->ProcessStream(
			(const int16_t*)(CombinedBuf.GetData() + Pos), // src
			input_config,
			output_config,
			(int16_t*)(PCMData.GetData() + Pos)); // dst

		if (Impl->Vad)
		{
			Impl->Vad->ProcessChunk((int16_t*)(PCMData.GetData() + Pos), SamplesPerFrame, SampleRate);
		}

		Pos += (BytesPerFrame * NumChannels);
	}

	OldBuf.SetNumUninitialized(NRem, EAllowShrinking::No);
	if (NRem > 0)
	{
		FMemory::Memcpy(OldBuf.GetData(), CombinedBuf.GetData() + Pos, NRem);
	}

	if (Impl->Vad && NFrames > 0)
	{
		//VoiceProb = Impl->Vad->last_voice_probability();
		VoiceProb = 0.0f;
		const auto& Probs = Impl->Vad->chunkwise_voice_probabilities();
		if (!Probs.empty())
		{
			for (auto Prob : Probs)
			{
				VoiceProb += Prob;
			}
			VoiceProb /= (float)Probs.size();
		}
	}

	if (LevelControlEnabled)
	{
		RecommendedLevel = Impl->AudioProc->recommended_stream_analog_level() / 255.0f;
	}

	//AudioProcessingStats Stats = Impl->AudioProc->GetStatistics();

	return Pos; // NumBytesProcessed
}
