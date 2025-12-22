#pragma once

#include "Processors/HMITextToSpeech.h"
#include "HMITextToSpeech_Piper.generated.h"

USTRUCT()
struct HMIBACKEND_API FHMITextToSpeech_Piper_Operation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMITextToSpeechInput Input;
};

UCLASS()
class HMIBACKEND_API UHMITextToSpeech_Piper : 
	public UHMITextToSpeech,
	public THMIProcessorQueue<FHMITextToSpeech_Piper_Operation, FHMITextToSpeechInput>
{
	GENERATED_BODY()

	friend class UHMIBackendSubsystem;

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMITextToSpeech* GetOrCreateTTS_Piper(UObject* WorldContextObject, 
		FName Name = NAME_None,
		FString ModelName = TEXT(""),
		bool Accelerate = false,
		FString VoiceId = TEXT(""),
		float VoiceSpeed = 1.0f
	);

	UHMITextToSpeech_Piper(const FObjectInitializer& ObjectInitializer);
	UHMITextToSpeech_Piper(FVTableHelper& Helper);
	~UHMITextToSpeech_Piper();

	#if HMI_WITH_PIPER

	public:

	virtual bool IsProcessorImplemented() const override { return true; }
	virtual void StartOrWakeProcessor() override;
	virtual int64 ProcessInput(FHMITextToSpeechInput&& Input) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;

	protected:

	void SetPiperAPI(struct piper_api* Api) { PiperApi = Api; }

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	virtual void HandleOperationComplete(bool Success, FString&& Error, FHMITextToSpeechInput&& Input, FHMIWaveHandle&& Wave);

	int ModelSampleRate = 0;
	int ModelSampleBits = 0;
	int ModelNumChannels = 0;
	int ModelNumSpeakers = 0;

	void* PiperDll = nullptr;
	struct piper_api* PiperApi = nullptr;
	struct piper_context* PiperContext = nullptr;
	volatile int PiperCancelFlag = false;

	#endif // HMI_WITH_PIPER

	protected:

	UPROPERTY(Transient)
	TArray<FHMITextToSpeech_Piper_Operation> InputQueue;
};
