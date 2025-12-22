#pragma once 

#include "Processors/HMISpeechToText.h"
#include "ONNX/OnnxProvider.h"
#include "Containers/AnsiString.h"
#include "HMISpeechToText_Sherpa.generated.h"

USTRUCT()
struct HMIBACKEND_API FHMISpeechToText_Sherpa_Operation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMISpeechToTextInput Input;
};

UCLASS()
class HMIBACKEND_API UHMISpeechToText_Sherpa : 
	public UHMISpeechToText,
	public THMIProcessorQueue<FHMISpeechToText_Sherpa_Operation, FHMISpeechToTextInput>
{
	GENERATED_BODY()

	friend class UHMIBackendSubsystem;

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMISpeechToText* GetOrCreateSTT_Sherpa(UObject* WorldContextObject, 
		FName Name = NAME_None,
		FString ModelName = TEXT(""),
		EOnnxProvider Provider = EOnnxProvider::Cpu,
		int NumThreads = 4,
		FString DefaultLanguage = TEXT(""),
		bool IsLanguageDetector = false
	);

	UHMISpeechToText_Sherpa(const FObjectInitializer& ObjectInitializer);
	UHMISpeechToText_Sherpa(FVTableHelper& Helper);
	~UHMISpeechToText_Sherpa();

	#if HMI_WITH_SHERPA

	public:

	virtual bool IsProcessorImplemented() const override { return true; }
	virtual int64 ProcessInput(FHMISpeechToTextInput&& Input) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;

	protected:

	void SetSherpaAPI(const struct FSherpaAPI* InSherpaAPI) { SherpaAPI = InSherpaAPI; }

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	virtual void HandleOperationComplete(bool Success, FString&& Error, FHMISpeechToTextInput&& Input, FString&& OutputText, FName&& OutputLanguage);

	int ModelSampleRate = 0;
	TMap<FAnsiString, FAnsiString> SherpaStrings;
	const struct FSherpaAPI* SherpaAPI = nullptr;
	const struct SherpaOnnxSpokenLanguageIdentification* LanguageDetector = nullptr;
	const struct SherpaOnnxOfflineRecognizer* OfflineRecognizer = nullptr;
	const struct SherpaOnnxOnlineRecognizer* OnlineRecognizer = nullptr;
	const struct SherpaOnnxOnlineStream* OnlineStream = nullptr;
	TUniquePtr<class FHMIResampler> Resampler;
	TArray<float> PaddedPcmF32;
	TArray<float> ResampledPcmF32;

	#endif // HMI_WITH_SHERPA

	protected:

	UPROPERTY(Transient)
	TArray<FHMISpeechToText_Sherpa_Operation> InputQueue;
};
