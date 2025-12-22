#pragma once

#include "Components/HMIWavePlayerComponent.h"
#include "HMILipSyncComponent.generated.h"

UCLASS(BlueprintType, ClassGroup="HMI|LipSync", meta=(BlueprintSpawnableComponent))
class HMI_API UHMILipSyncComponent : public UActorComponent
{
	GENERATED_BODY()

	public:

	UHMILipSyncComponent(const FObjectInitializer& ObjectInitializer);
	UHMILipSyncComponent(FVTableHelper& Helper);
	~UHMILipSyncComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|LipSync", meta=(ClampMin="0.0", ClampMax="0.5", UIMin="0.0", UIMax="0.5"))
	float TrimLastSeconds = 0.1f;

	UFUNCTION(BlueprintCallable, Category="HMI|LipSync")
	void SetWavePlayer(UHMIWavePlayerComponent* InPlayer);

	UFUNCTION(BlueprintCallable, Category="HMI|LipSync")
	void ResetLipSync();

	UFUNCTION(BlueprintPure, Category="HMI|LipSync")
	int GetFrameId() const { return FrameId; }

	UFUNCTION(BlueprintPure, Category="HMI|LipSync")
	const FHMILipSyncFrame& GetFrame() const { return Frame; }

	UFUNCTION(BlueprintPure, Category="HMI|LipSync")
	const TArray<float>& GetWeights() const { return Frame.Weights; }

	UFUNCTION(BlueprintPure, Category="HMI|LipSync")
	float GetWeight(int Id) const { return Frame.GetWeight(Id); }

	UFUNCTION(BlueprintPure, Category="HMI|LipSync")
	float GetWeightByName(FName Name) const { return Frame.GetWeight(Name); }

	protected:

	// UHMIWavePlayerComponent
	UFUNCTION()
	void OnWaveStart(const FHMIPlayWaveInput& Input);
	UFUNCTION()
	void OnWaveStop(const FHMIPlayWaveInput& Input, const FHMIPlayWaveResult& Result);
	UFUNCTION()
	void OnWaveProgress(bool WasStarted, float ProgressSeconds, float DurationSeconds);
	UFUNCTION()
	void OnPlayerUnderflow();
	UFUNCTION()
	void OnPlayerStop();

	void SubmitFrame();

	protected:

	UPROPERTY(Transient)
	TObjectPtr<class UHMIPoseRemapComponent> RemapComp;

	UPROPERTY(Transient)
	TObjectPtr<UHMIWavePlayerComponent> WavePlayer;

	UPROPERTY(Transient)
	FHMILipSyncSequenceHandle Sequence;

	UPROPERTY(Transient)
	FHMILipSyncFrame Frame;

	int FrameId = 0;
};
