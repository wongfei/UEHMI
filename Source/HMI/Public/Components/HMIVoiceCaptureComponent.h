#pragma once

#include "HMIBuffer.h"
#include "Components/ActorComponent.h" 
#include "Audio/HMIAudioProcessor.h"
#include "HMIVoiceCaptureComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMIOnVoiceCaptured, const TArray<uint8>&, Buffer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMIOnSentenceCaptured, FHMIWaveHandle, Wave);

UCLASS(BlueprintType, ClassGroup="HMI|VoiceCapture", meta=(BlueprintSpawnableComponent))
class HMI_API UHMIVoiceCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

	public:

	UHMIVoiceCaptureComponent(const FObjectInitializer& ObjectInitializer);
	UHMIVoiceCaptureComponent(FVTableHelper& Helper);
	~UHMIVoiceCaptureComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	public:

	// Device

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device")
	bool AutostartCapture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device")
	FString DeviceName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device", meta=(ClampMin="8000", ClampMax="48000", UIMin="8000", UIMax="48000"))
	int SampleRate; // 0 -> HMI_DEFAULT_SAMPLE_RATE
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device", meta=(ClampMin="0.001", ClampMax="0.5", UIMin="0.001", UIMax="0.5"))
	float PollRate = 0.01f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device", meta=(ClampMin="0.0", ClampMax="20.0", UIMin="0.0", UIMax="20.0"))
	float InputGain = 1.0f;
	const float InputGainRange = 20.0f; // same as InputGain Min/Max

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device", meta=(ClampMin="0.0", ClampMax="0.5", UIMin="0.0", UIMax="0.5"))
	float JitterBufferDelay = 0.2f; // UE 0.3f

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Device", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float MinVoiceProbability = 0.6f;

	// Silence detection

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Silence", meta=(DisplayName="Threshold", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float SilenceThreshold = 0.01f; // UE 0.08
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Silence", meta=(DisplayName="AttackTime", ClampMin="0.0", ClampMax="500.0", UIMin="0.0", UIMax="500.0"))
	float SilenceAttackTime = 10.0f; // UE 2.0
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Silence", meta=(DisplayName="ReleaseTime", ClampMin="0.0", ClampMax="2000.0", UIMin="0.0", UIMax="2000.0"))
	float SilenceReleaseTime = 250.0f; // UE 1100.0

	// Noise

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Noise", meta=(DisplayName="Threshold", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float NoiseThreshold = 0.01f; // UE 0.08
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Noise", meta=(DisplayName="AttackTime", ClampMin="0.0", ClampMax="0.5", UIMin="0.0", UIMax="0.5"))
	float NoiseAttackTime = 0.01f; // UE 0.05
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Noise", meta=(DisplayName="ReleaseTime", ClampMin="0.0", ClampMax="2.0", UIMin="0.0", UIMax="2.0"))
	float NoiseReleaseTime = 0.25f; // UE 0.30

	// Sentence detection

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Sentence", meta=(DisplayName="Min Duration", ClampMin="0.0", ClampMax="10.0", UIMin="0.0", UIMax="10.0"))
	float SentenceMinimalDuration = 0.33f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Sentence", meta=(DisplayName="Mic Cutoff", ClampMin="0.0", ClampMax="10.0", UIMin="0.0", UIMax="10.0"))
	float SentenceMicCutoff = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Sentence", meta=(DisplayName="Voice Cutoff", ClampMin="0.0", ClampMax="10.0", UIMin="0.0", UIMax="10.0"))
	float SentenceVoiceCutoff = 1.2f;

	// Audio processor

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Processor", meta=(DisplayName="Enable"))
	bool EnableAudioProcessor = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|VoiceCapture|Processor", meta=(DisplayName="Config"))
	FHMIAudioProcessorConfig ProcessorConfig;

	// Start/Stop

	UFUNCTION(BlueprintCallable, Category="HMI|VoiceCapture")
	void WarmUp();

	UFUNCTION(BlueprintCallable, Category="HMI|VoiceCapture")
	void StartCapture();
	
	UFUNCTION(BlueprintCallable, Category="HMI|VoiceCapture")
	void StopCapture();

	/* WARNING: AVOID USING THIS IN BP */
	UPROPERTY(BlueprintAssignable, Category="HMI|VoiceCapture")
	FHMIOnVoiceCaptured OnVoiceCaptured;
	
	UPROPERTY(BlueprintAssignable, Category="HMI|VoiceCapture")
	FHMIOnSentenceCaptured OnSentenceCaptured;

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	const TArray<uint8>& GetVoiceBuffer() const { return VoiceBuffer; }

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	double GetTimestamp() const;

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	float GetCurrentAmplitude() const;

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	float GetSentenceDuration() const;

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	double GetLastMicActivity() const { return LastMicActivity; }

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	double GetLastVoiceActivity() const { return LastVoiceActivity; }

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	double GetFirstVoiceActivity() const { return FirstVoiceActivity; }

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	float GetVoiceProb() const { return VoiceProb; }

	UFUNCTION(BlueprintPure, Category="HMI|VoiceCapture")
	bool IsSentenceStarted() const { return bSentenceStarted; }

	protected:

	void PollDevice();
	void HandleVoiceCaptured();
	void HandleSentenceCaptured(float Duration);

	void PermissionCallback(const TArray<FString>& Permissions, const TArray<bool>& GrantResults);
	void StartCaptureDevice();
	void StopCaptureDevice();
	void DetectSentence();

	protected:

	FHMIWaveFormat OutputFormat;
	TSharedPtr<class IVoiceCapture> VoiceCapture;
	TUniquePtr<class FHMIAudioProcessor> AudioProcessor;
	FTimerHandle VoiceCaptureTimer;

	UPROPERTY(Transient)
	TArray<uint8> VoiceBuffer;

	UPROPERTY(Transient)
	TArray<uint8> SentenceBuffer;

	bool bIsWarmingUp = false;
	bool bSentenceStarted = false;
	double LastMicActivity = 0;
	double LastVoiceActivity = 0;
	double FirstVoiceActivity = 0;
	float VoiceProb = 0;
};
