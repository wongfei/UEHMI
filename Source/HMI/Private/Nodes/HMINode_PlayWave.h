#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Components/HMIWavePlayerComponent.h"
#include "HMINode_PlayWave.generated.h"

UCLASS()
class HMI_API UHMINode_PlayWave : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnWaveStop Complete;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="PlayWave", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UHMINode_PlayWave* PlayWave_Async(
		UObject* WorldContextObject, UHMIWavePlayerComponent* Player, 
		FName UserTag, FHMIWaveHandle Wave, FHMILipSyncSequenceHandle Sequence);

	virtual void Activate() override;

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	UHMIWavePlayerComponent* Player;

	UPROPERTY(Transient)
	FHMIWaveHandle Wave;

	UPROPERTY(Transient)
	FHMILipSyncSequenceHandle Sequence;

	UPROPERTY(Transient)
	FName UserTag;
};
