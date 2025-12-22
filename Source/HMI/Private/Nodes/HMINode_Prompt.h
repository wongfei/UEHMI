#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Processors/HMIChat.h"
#include "HMINode_Prompt.generated.h"

UCLASS()
class HMI_API UHMINode_Prompt : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:
	
	UPROPERTY(BlueprintAssignable)
	FHMIOnChatBatch Progress;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="Prompt", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="History,BackendParams"))
	static UHMINode_Prompt* Prompt_Async(
		UObject* WorldContextObject, UHMIProcessor* Processor, 
		FName UserTag, FString Text, 
		TArray<FHMIChatMessage> History, TMap<FString, FString> BackendParams,
		int MaxTokens = 0, float Temperature = 1.0f, float TopP = 1.0f, bool Streaming = true
	);

	virtual void Activate() override;

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	UHMIChat* Processor;

	UPROPERTY(Transient)
	FName UserTag;

	UPROPERTY(Transient)
	FString Text;

	UPROPERTY(Transient)
	TArray<FHMIChatMessage> History;

	UPROPERTY(Transient)
	TMap<FString, FString> BackendParams;

	UPROPERTY(Transient)
	int MaxTokens;

	UPROPERTY(Transient)
	int Temperature;

	UPROPERTY(Transient)
	int TopP;

	UPROPERTY(Transient)
	bool Streaming;
};
