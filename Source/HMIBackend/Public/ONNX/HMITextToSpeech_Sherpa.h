#pragma once

#include "Processors/HMITextToSpeech.h"
#include "ONNX/OnnxProvider.h"
#include "Containers/AnsiString.h"
#include "HMITextToSpeech_Sherpa.generated.h"

USTRUCT()
struct HMIBACKEND_API FHMITextToSpeech_Sherpa_Operation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMITextToSpeechInput Input;
};

UCLASS()
class HMIBACKEND_API UHMITextToSpeech_Sherpa : 
	public UHMITextToSpeech,
	public THMIProcessorQueue<FHMITextToSpeech_Sherpa_Operation, FHMITextToSpeechInput>
{
	GENERATED_BODY()

	friend class UHMIBackendSubsystem;

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMITextToSpeech* GetOrCreateTTS_Sherpa(UObject* WorldContextObject, 
		FName Name = NAME_None,
		FString ModelName = TEXT(""),
		EOnnxProvider Provider = EOnnxProvider::Cpu,
		int NumThreads = 1,
		FString VoiceId = TEXT(""),
		float VoiceSpeed = 1.0f
	);

	UHMITextToSpeech_Sherpa(const FObjectInitializer& ObjectInitializer);
	~UHMITextToSpeech_Sherpa();

	#if HMI_WITH_SHERPA

	public:

	virtual bool IsProcessorImplemented() const override { return true; }
	virtual int64 ProcessInput(FHMITextToSpeechInput&& Input) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;

	protected:

	void SetSherpaAPI(const struct FSherpaAPI* InSherpaAPI) { SherpaAPI = InSherpaAPI; }

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	virtual void HandleOperationComplete(bool Success, FString&& Error, FHMITextToSpeechInput&& Input, FHMIWaveHandle&& Wave);

	TMap<FAnsiString, FAnsiString> SherpaStrings;
	const struct FSherpaAPI* SherpaAPI = nullptr;
	const struct SherpaOnnxOfflineTts* tts = nullptr;

	#endif // HMI_WITH_SHERPA

	protected:

	UPROPERTY(Transient)
	TArray<FHMITextToSpeech_Sherpa_Operation> InputQueue;
};
