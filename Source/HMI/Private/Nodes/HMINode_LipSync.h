#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Processors/HMILipSync.h"
#include "HMINode_LipSync.generated.h"

UCLASS()
class HMI_API UHMINode_LipSync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnLipSyncComplete Complete;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="LipSync", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UHMINode_LipSync* LipSync_Async(
		UObject* WorldContextObject, UHMIProcessor* Processor, 
		FName UserTag, FHMIWaveHandle Wave);

	virtual void Activate() override;

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	UHMILipSync* Processor;

	UPROPERTY(Transient)
	FHMIWaveHandle Wave;

	UPROPERTY(Transient)
	FName UserTag;
};
