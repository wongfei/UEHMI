#include "Processors/HMITextToSpeech.h"
#include "Audio/HMIResampler.h"

#include "DSP/FloatArrayMath.h"

UHMITextToSpeech::UHMITextToSpeech(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetProcessorParam("OutputSampleRate", HMI_DEFAULT_SAMPLE_RATE);
	SetProcessorParam("OutputClampAbs", 0.9f);
}

UHMITextToSpeech::UHMITextToSpeech(FVTableHelper& Helper) : Super(Helper)
{
}

UHMITextToSpeech::~UHMITextToSpeech()
{
}

bool UHMITextToSpeech::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	OutputSampleRate = FMath::Clamp(GetProcessorInt("OutputSampleRate"), 0, 48000);
	OutputClampAbs = FMath::Clamp(GetProcessorFloat("OutputClampAbs"), 0.0f, 1.0f);

	if (!OutputSampleRate) 
		OutputSampleRate = HMI_DEFAULT_SAMPLE_RATE;

	if (!UHMIBufferStatics::IsValidSampleRate(OutputSampleRate))
	{
		ProcError(FString::Printf(TEXT("Invalid sample rate: %d"), OutputSampleRate));
		return false;
	}

	return true;
}

FHMIWaveHandle UHMITextToSpeech::GenerateOutputWave(TArray<float>& Samples, int SampleRate, int NumChannels)
{
	if (OutputClampAbs > 0.0f)
	{
		Audio::ArrayClampInPlace(Samples, -OutputClampAbs, OutputClampAbs);
	}

	if (OutputSampleRate && OutputSampleRate != SampleRate)
	{
		if (!Resampler)
		{
			Resampler = MakeUnique<FHMIResampler>();
			Resampler->InitDefaultVoice();
		}

		Resampler->Convert(Samples, SampleRate, TmpS16, OutputSampleRate);
	}
	else
	{
		FPcm::F32_S16(Samples, TmpS16);
	}

	auto Wave = FHMIWaveHandle::Alloc_PCM_S16(OutputSampleRate, NumChannels);
	Wave.CopyFrom(TmpS16);

	return Wave;
}

FHMIWaveHandle UHMITextToSpeech::GenerateOutputWave(TArrayView<const float> Samples, int SampleRate, int NumChannels)
{
	TmpF32 = Samples;
	return GenerateOutputWave(TmpF32, SampleRate, NumChannels);
}

FHMIWaveHandle UHMITextToSpeech::GenerateOutputWave(TArrayView<const int16> Samples, int SampleRate, int NumChannels)
{
	FPcm::S16_F32(Samples, TmpF32);
	return GenerateOutputWave(TmpF32, SampleRate, NumChannels);
}
