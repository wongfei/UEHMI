#pragma once

#include "HMIProcessor.h"
#include "HMILipSyncSequence.h"
#include "HMILipSync.generated.h"

USTRUCT(BlueprintType, Category="HMI|LipSync")
struct HMI_API FHMILipSyncInput : public FHMIProcessorInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FHMIWaveHandle Wave;

	FHMILipSyncInput() = default;

	FHMILipSyncInput(FName InUserTag, FHMIWaveHandle InWave)
	{
		UserTag = MoveTemp(InUserTag);
		Wave = MoveTemp(InWave);
	}
};

USTRUCT(BlueprintType, Category="HMI|LipSync")
struct HMI_API FHMILipSyncResult : public FHMIProcessorResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FHMILipSyncSequenceHandle Sequence;

	FHMILipSyncResult() = default;

	FHMILipSyncResult(double InTimestamp, bool InSuccess, FString InError, FHMILipSyncSequenceHandle InSequence) : 
		FHMIProcessorResult(InTimestamp, InSuccess, MoveTemp(InError)),
		Sequence(MoveTemp(InSequence))
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMIOnLipSyncComplete, const FHMILipSyncInput&, Input, const FHMILipSyncResult&, Result);

UCLASS(BlueprintType, Abstract, ClassGroup="HMI|LipSync")
class HMI_API UHMILipSync : public UHMIProcessor
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable, Category="HMI|LipSync")
	FHMIOnLipSyncComplete OnLipSyncComplete;

	UFUNCTION(BlueprintCallable, Category="HMI|LipSync")
	virtual int64 ProcessWave(FName UserTag, FHMIWaveHandle Wave)
	{
		return ProcessInput(FHMILipSyncInput(MoveTemp(UserTag), MoveTemp(Wave)));
	}

	virtual int64 ProcessInput(FHMILipSyncInput&& Input) { return 0; }
};
