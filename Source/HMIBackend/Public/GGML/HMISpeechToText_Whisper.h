#pragma once 

#include "Processors/HMISpeechToText.h"
#include "HMISpeechToText_Whisper.generated.h"

USTRUCT()
struct HMIBACKEND_API FHMISpeechToText_Whisper_Operation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMISpeechToTextInput Input;
};

UCLASS()
class HMIBACKEND_API UHMISpeechToText_Whisper : 
	public UHMISpeechToText,
	public THMIProcessorQueue<FHMISpeechToText_Whisper_Operation, FHMISpeechToTextInput>
{
	GENERATED_BODY()

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMISpeechToText* GetOrCreateSTT_Whisper(UObject* WorldContextObject, 
		FName Name = NAME_None,
		FString ModelName = TEXT(""),
		bool Accelerate = true,
		int NumThreads = 4,
		FString DefaultLanguage = TEXT(""),
		bool IsLanguageDetector = false
	);

	UHMISpeechToText_Whisper(const FObjectInitializer& ObjectInitializer);
	UHMISpeechToText_Whisper(FVTableHelper& Helper);
	~UHMISpeechToText_Whisper();

	#if HMI_WITH_WHISPER

	public:

	virtual bool IsProcessorImplemented() const override { return true; }
	virtual int64 ProcessInput(FHMISpeechToTextInput&& Input) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;
	virtual bool GetCancelFlag() const { return CancelFlag.load(); }
	
	protected:

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	virtual void HandleOperationComplete(bool Success, FString&& Error, FHMISpeechToTextInput&& Input, FString&& OutputText, FName&& OutputLanguage);

	int NumProcessors = 0;
	int NumThreads = 0;

	struct whisper_full_params* FullParams = nullptr;
	struct whisper_context* Context = nullptr;
	TUniquePtr<class FHMIResampler> Resampler;
	TArray<float> PaddedPcmF32;
	TArray<float> ResampledPcmF32;

	#endif // HMI_WITH_WHISPER

	protected:

	UPROPERTY(Transient)
	TArray<FHMISpeechToText_Whisper_Operation> InputQueue;
};
