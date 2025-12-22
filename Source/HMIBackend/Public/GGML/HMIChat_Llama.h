#pragma once

#include "Processors/HMIChat.h"
#include "HMIChat_Llama.generated.h"

USTRUCT()
struct HMIBACKEND_API FHMIChat_Llama_Operation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMIChatInput Input;
};

UCLASS()
class HMIBACKEND_API UHMIChat_Llama : 
	public UHMIChat, 
	public THMIProcessorQueue<FHMIChat_Llama_Operation, FHMIChatInput>
{
	GENERATED_BODY()

	friend class FLlamaContext;

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMIChat* GetOrCreateChat_Llama(UObject* WorldContextObject, 
		FName Name = NAME_None,
		FString ModelName = TEXT(""),
		int ContextSize = 0,
		int NumThreads = 4,
		int NumGpuLayers = 0
	);

	UHMIChat_Llama(const FObjectInitializer& ObjectInitializer);
	UHMIChat_Llama(FVTableHelper& Helper);
	~UHMIChat_Llama();

	#if HMI_WITH_LLAMA

	public:
	
	virtual bool IsProcessorImplemented() const override { return true; }
	virtual int64 ProcessInput(FHMIChatInput&& Input) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;

	protected:

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	TUniquePtr<class FLlamaContext> Context;

	#endif // HMI_WITH_LLAMA

	protected:

	UPROPERTY(Transient)
	TArray<FHMIChat_Llama_Operation> InputQueue;
};
