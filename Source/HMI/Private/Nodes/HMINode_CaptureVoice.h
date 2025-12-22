#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Components/HMIVoiceCaptureComponent.h"
#include "HMINode_CaptureVoice.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMIOnVoiceCaptureNode, FHMIWaveHandle, Wave);

UCLASS()
class HMI_API UHMINode_CaptureVoice : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnVoiceCaptureNode Complete;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="CaptureVoice", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UHMINode_CaptureVoice* CaptureVoice_Async(UObject* WorldContextObject, UHMIVoiceCaptureComponent* Capture, FName UserTag);

	virtual void Activate() override;

	UFUNCTION()
	void OnSentenceCaptured(FHMIWaveHandle Wave);

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	UHMIVoiceCaptureComponent* Capture;

	UPROPERTY(Transient)
	FName UserTag;

	bool RegisteredFlag = false;
};
