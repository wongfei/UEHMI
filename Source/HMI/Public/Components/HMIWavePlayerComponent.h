#pragma once

#include "HMIBuffer.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWaveProcedural.h"
#include "Processors/HMIProcessorInput.h"
#include "Processors/HMIProcessorResult.h"
#include "Processors/HMIProcessorQueue.h"
#include "Processors/HMILipSyncSequence.h"
#include "HMIWavePlayerComponent.generated.h"

USTRUCT(BlueprintType, Category="HMI|WavePlayer")
struct HMI_API FHMIPlayWaveInput : public FHMIProcessorInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FHMIWaveHandle Wave;

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FHMILipSyncSequenceHandle Sequence;

	FHMIPlayWaveInput() = default;

	FHMIPlayWaveInput(FName InUserTag, FHMIWaveHandle InWave, FHMILipSyncSequenceHandle InSequence)
	{
		UserTag = MoveTemp(InUserTag);
		Wave = MoveTemp(InWave);
		Sequence = MoveTemp(InSequence);
	}
};

USTRUCT(BlueprintType, Category="HMI|WavePlayer")
struct HMI_API FHMIPlayWaveResult : public FHMIProcessorResult
{
	GENERATED_BODY()

	FHMIPlayWaveResult() = default;

	FHMIPlayWaveResult(double InTimestamp, bool InSuccess) : FHMIProcessorResult(InTimestamp, InSuccess) {}
};

USTRUCT()
struct HMI_API FHMIPlayWaveOperation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMIPlayWaveInput Input;

	double StartPlayTS = 0;
	double FinishPlayTS = 0;
	bool IsPlaying = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMIOnWaveStart, const FHMIPlayWaveInput&, Input);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHMIOnWaveProgress, bool, WasStarted, float, ProgressSeconds, float, DurationSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMIOnWaveStop, const FHMIPlayWaveInput&, Input, const FHMIPlayWaveResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHMIOnPlayerUnderflow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHMIOnPlayerStop);

UCLASS(BlueprintType, ClassGroup="HMI|WavePlayer", meta=(BlueprintSpawnableComponent))
class HMI_API UHMIWavePlayerComponent : 
	public USceneComponent,
	public THMIProcessorQueue<FHMIPlayWaveOperation, FHMIPlayWaveInput>
{
	GENERATED_BODY()

	public:

	UHMIWavePlayerComponent(const FObjectInitializer& ObjectInitializer);
	UHMIWavePlayerComponent(FVTableHelper& Helper);
	~UHMIWavePlayerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	public: 

	// Settings

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer", meta=(ClampMin="8000", ClampMax="48000", UIMin="8000", UIMax="48000"))
	int OutputSampleRate; // 0 -> HMI_DEFAULT_SAMPLE_RATE

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer", meta=(ClampMin="0.0", ClampMax="20.0", UIMin="0.0", UIMax="20.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer", meta=(ClampMin="0.01", ClampMax="0.5", UIMin="0.01", UIMax="0.5"))
	float FadeDurationSec = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer", meta=(ClampMin="1.0", ClampMax="100.0", UIMin="1.0", UIMax="100.0"))
	float AutostopAfterInactiveSec = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer", meta=(ClampMin="0.01", ClampMax="0.2", UIMin="0.01", UIMax="0.2"))
	float MinPrefetchSec = 0.01666f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer")
	bool EnableDebugBuffer;

	UPROPERTY(EditAnywhere, Category="HMI|WavePlayer")
	TObjectPtr<class USoundClass> SoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer")
	TObjectPtr<class USoundAttenuation> AttenuationSettings;

	// Effects

	UPROPERTY(EditAnywhere, Category="HMI|WavePlayer|FX")
	uint32 bEnableBaseSubmix : 1 = true;

	UPROPERTY(EditAnywhere, Category="HMI|WavePlayer|FX")
	uint32 bEnableSubmixSends : 1 = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer|FX")
	uint32 bEnableBusSends : 1 = true;

	UPROPERTY(EditAnywhere, Category="HMI|WavePlayer|FX")
	TObjectPtr<class USoundEffectSourcePresetChain> SourceEffectChain;

	UPROPERTY(EditAnywhere, Category="HMI|WavePlayer|FX", meta=(EditCondition="bEnableBaseSubmix"))
	TObjectPtr<class USoundSubmixBase> SoundSubmix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer|FX", meta=(EditCondition="bEnableSubmixSends"))
	TArray<struct FSoundSubmixSendInfo> SoundSubmixSends;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer|FX", meta=(EditCondition="bEnableBusSends"))
	TArray<struct FSoundSourceBusSendInfo> PreEffectBusSends;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|WavePlayer|FX", meta=(EditCondition="bEnableBusSends"))
	TArray<struct FSoundSourceBusSendInfo> PostEffectBusSends;

	// Play/Stop

	int64 ProcessInput(FHMIPlayWaveInput&& Input);

	UFUNCTION(BlueprintCallable, Category="HMI|WavePlayer")
	int64 Play(FName UserTag, FHMIWaveHandle Wave, FHMILipSyncSequenceHandle Sequence)
	{
		return ProcessInput(FHMIPlayWaveInput(MoveTemp(UserTag), MoveTemp(Wave), MoveTemp(Sequence)));
	}

	UFUNCTION(BlueprintCallable, Category="HMI|WavePlayer")
	void Stop(bool FadeOut = false);

	UFUNCTION(BlueprintPure, Category="HMI|WavePlayer")
	bool IsPlaying() const;

	UFUNCTION(BlueprintPure, Category="HMI|WavePlayer")
	double GetAudioTS() const;

	UPROPERTY(BlueprintAssignable, Category="HMI|WavePlayer")
	FHMIOnWaveStart OnWaveStart;

	UPROPERTY(BlueprintAssignable, Category="HMI|WavePlayer")
	FHMIOnWaveProgress OnWaveProgress;

	UPROPERTY(BlueprintAssignable, Category="HMI|WavePlayer")
	FHMIOnWaveStop OnWaveStop;

	UPROPERTY(BlueprintAssignable, Category="HMI|WavePlayer")
	FHMIOnPlayerUnderflow OnPlayerUnderflow;

	UPROPERTY(BlueprintAssignable, Category="HMI|WavePlayer")
	FHMIOnPlayerStop OnPlayerStop;

	UFUNCTION(BlueprintPure, Category="HMI|WavePlayer")
	class UAudioComponent* GetAudioComponent() const { return AudioComponent; }

	UFUNCTION(BlueprintPure, Category="HMI|WavePlayer")
	class USoundWaveProcedural* GetProceduralWave() const { return ProceduralWave; }

	UFUNCTION(BlueprintPure, Category="HMI|WavePlayer")
	const struct FHMIWaveHandle& GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category="HMI|WavePlayer")
	const struct FHMILipSyncSequenceHandle& GetCurrentSequence() const { return CurrentSequence; }

	protected:

	void CreateAudioComponent();
	void DestroyAudioComponent();

	void PlayNextBuffer();
	void TickPlaybackProgress(float DeltaTime);
	void ResetAudio();

	void QueueBytes(const uint8* Data, int NumBytes);
	void QueueSamples(const int16* Data, int NumSamples);
	void Fadein(const TArrayView<const int16>& Wave);
	void Fadeout(const TArrayView<const int16>& Wave);

	void OnAudioPlayStateChanged(const UAudioComponent* Comp, EAudioComponentPlayState NewState);
	void OnAudioPlaybackPercent(const UAudioComponent* Comp, const USoundWave* Wave, const float Percent);
	void OnAudioPlaybackFinished(UAudioComponent* Comp);

	void HandleOperationComplete(FHMIPlayWaveInput& Input, bool Success);

	protected:

	UPROPERTY(Transient)
	class UAudioComponent* AudioComponent;

	UPROPERTY(Transient)
	class USoundWaveProcedural* ProceduralWave;

	UPROPERTY(Transient)
	TArray<FHMIPlayWaveOperation> InputQueue;

	UPROPERTY(Transient)
	TArray<FHMIPlayWaveOperation> PendingOperations;

	TArray<int> CompletedOperations;
	
	UPROPERTY(Transient)
	FHMIWaveHandle CurrentWave;

	UPROPERTY(Transient)
	FHMILipSyncSequenceHandle CurrentSequence;

	TUniquePtr<class FHMIResampler> Resampler;
	TArray<float> PcmF32;
	TArray<int16> PcmS16;
	TArray<int16> FadeBuffer;
	TArray<uint8> DebugBuffer;

	EAudioComponentPlayState PlayState = EAudioComponentPlayState::Stopped;
	double PlaybackTS = 0;
	double UnderflowTS = 0;
	double PrefetchTS = 0;
	double AutostopTS = 0;
	double LastReportedUnderflowTS = 0;
};
