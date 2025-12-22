#pragma once

#include "HMIMinimal.h"
#include "HMIAudioProcessor.generated.h"

USTRUCT(BlueprintType, Category="HMI|Audio")
struct HMI_API FHMIAudioProcessorConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	bool HighPassFilter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	bool EchoCanceller = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	bool EchoCancellerMobile = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	bool NoiseSuppression = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	bool GainController = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	bool TransientSuppression = false;
};

class HMI_API FHMIAudioProcessor
{
	public:

	FHMIAudioProcessor(const FHMIAudioProcessorConfig& InConfig);
	~FHMIAudioProcessor();
	
	// returns NumBytesProcessed
	int Process(TArray<uint8>& PCMData, int NumSamples, int SampleRate, int NumChannels = 1);

	void Flush();
	
	bool IsLevelControlEnabled() const { return LevelControlEnabled; } 
	void SetCurrentLevel(float Level) { CurrentLevel = Level; } 
	float GetRecommendedLevel() const { return RecommendedLevel; }
	float GetVoiceProb() const { return VoiceProb; }

	protected:

	TUniquePtr<struct FHMIAudioProcessorImpl> Impl;
	TArray<uint8> CombinedBuf;
	TArray<uint8> OldBuf;

	bool LevelControlEnabled = false;
	float CurrentLevel = 0.01f;
	float RecommendedLevel = 0.01f;
	float VoiceProb = 0.0f;
};
