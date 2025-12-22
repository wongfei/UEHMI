#include "Audio/HMIResampler.h"
#include "Audio/HMIPcm.h"

#include "DSP/FloatArrayMath.h"

DECLARE_CYCLE_STAT(TEXT("ConvertAudio"), STAT_HMI_ConvertAudio, STATGROUP_HMI)

FHMIResampler::FHMIResampler()
{
	Impl = MakeUnique<Audio::FResampler>();
}

FHMIResampler::~FHMIResampler()
{
}

void FHMIResampler::Init(Audio::EResamplingMethod InResamplingMethod, float InSampleRateRatio, int32 InNumChannels)
{
	ResamplingMethod = InResamplingMethod;
	NumChannels = InNumChannels;

	Impl->Init(InResamplingMethod, InSampleRateRatio, InNumChannels);
}

void FHMIResampler::InitDefaultVoice()
{
	Init(Audio::EResamplingMethod::Linear, 1.0f, HMI_VOICE_CHANNELS);
}

void FHMIResampler::SetSampleRateRatio(float InRatio)
{
	Impl->SetSampleRateRatio(InRatio);
}

void FHMIResampler::ProcessAudio(const float* InAudioBuffer, int32 InSamples, bool bEndOfInput, float* OutAudioBuffer, int32 MaxOutputFrames, int32& OutNumFrames)
{
	// InAudioBuffer should be const
	Impl->ProcessAudio(const_cast<float*>(InAudioBuffer), InSamples, bEndOfInput, OutAudioBuffer, MaxOutputFrames, OutNumFrames);
}

void FHMIResampler::ConvertImpl(const TArrayView<const float>& Src, int SrcRate, TArray<float>& Dst, int DstRate)
{
	const float Ratio = (float)DstRate / (float)SrcRate;
	const int EstimatedSamples = (int)(Src.Num() * Ratio) + 16;
	Dst.SetNumUninitialized(EstimatedSamples, EAllowShrinking::No);

	int32 OutNumFrames = 0;
	Init(ResamplingMethod, Ratio, NumChannels);
	ProcessAudio(Src.GetData(), Src.Num(), false, Dst.GetData(), Dst.Num(), OutNumFrames);

	Dst.SetNum(OutNumFrames, EAllowShrinking::No);
}

void FHMIResampler::Convert(const TArrayView<const float>& Src, int SrcRate, TArray<float>& Dst, int DstRate)
{
	SCOPE_CYCLE_COUNTER(STAT_HMI_ConvertAudio)

	ConvertImpl(Src, SrcRate, Dst, DstRate);
}

void FHMIResampler::Convert(const TArrayView<const int16>& Src, int SrcRate, TArray<int16>& Dst, int DstRate)
{
	SCOPE_CYCLE_COUNTER(STAT_HMI_ConvertAudio)

	FPcm::S16_F32(Src, SrcF32);
	ConvertImpl(SrcF32, SrcRate, DstF32, DstRate);
	FPcm::F32_S16(DstF32, Dst);
}

void FHMIResampler::Convert(const TArrayView<const float>& Src, int SrcRate, TArray<int16>& Dst, int DstRate)
{
	SCOPE_CYCLE_COUNTER(STAT_HMI_ConvertAudio)

	ConvertImpl(Src, SrcRate, DstF32, DstRate);
	FPcm::F32_S16(DstF32, Dst);
}

void FHMIResampler::Convert(const TArrayView<const int16>& Src, int SrcRate, TArray<float>& Dst, int DstRate)
{
	SCOPE_CYCLE_COUNTER(STAT_HMI_ConvertAudio)

	FPcm::S16_F32(Src, SrcF32);
	ConvertImpl(SrcF32, SrcRate, Dst, DstRate);
}
