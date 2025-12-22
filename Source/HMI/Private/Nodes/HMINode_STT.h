#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Processors/HMISpeechToText.h"
#include "HMINode_STT.generated.h"

UCLASS()
class HMI_API UHMINode_STT : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnSpeechToTextComplete Complete;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="STT", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UHMINode_STT* STT_Async(
		UObject* WorldContextObject, UHMIProcessor* Processor, 
		FName UserTag, FHMIWaveHandle Wave, FName Language);

	virtual void Activate() override;

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	UHMISpeechToText* Processor;

	UPROPERTY(Transient)
	FName UserTag;

	UPROPERTY(Transient)
	FHMIWaveHandle Wave;

	UPROPERTY(Transient)
	FName Language;
};
