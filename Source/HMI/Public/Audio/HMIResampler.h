#pragma once

#include "HMIMinimal.h"
#include "AudioResampler.h"

class HMI_API FHMIResampler
{
	public:
	
	FHMIResampler();
	~FHMIResampler();
	
	void Init(Audio::EResamplingMethod InResamplingMethod, float InSampleRateRatio, int32 InNumChannels);
	void InitDefaultVoice();
	void SetSampleRateRatio(float InRatio);

	void ProcessAudio(const float* InAudioBuffer, int32 InSamples, bool bEndOfInput, float* OutAudioBuffer, int32 MaxOutputFrames, int32& OutNumFrames);

	void Convert(const TArrayView<const float>& Src, int SrcRate, TArray<float>& Dst, int DstRate);
	void Convert(const TArrayView<const int16>& Src, int SrcRate, TArray<int16>& Dst, int DstRate);

	void Convert(const TArrayView<const float>& Src, int SrcRate, TArray<int16>& Dst, int DstRate);
	void Convert(const TArrayView<const int16>& Src, int SrcRate, TArray<float>& Dst, int DstRate);

	protected:
	
	void ConvertImpl(const TArrayView<const float>& Src, int SrcRate, TArray<float>& Dst, int DstRate);

	TUniquePtr<Audio::FResampler> Impl;
	Audio::EResamplingMethod ResamplingMethod = Audio::EResamplingMethod::Linear;
	int32 NumChannels = 0;
	TArray<float> SrcF32;
	TArray<float> DstF32;
};
