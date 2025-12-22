#pragma once

#include "HMIProcessor.h"
#include "HMISpeechToText.generated.h"

USTRUCT(BlueprintType, Category="HMI|SpeechToText")
struct HMI_API FHMISpeechToTextInput : public FHMIProcessorInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FHMIWaveHandle Wave;

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FName Language;

	FHMISpeechToTextInput() = default;

	FHMISpeechToTextInput(FName InUserTag, FHMIWaveHandle InWave, FName InLanguage)
	{
		UserTag = MoveTemp(InUserTag);
		Wave = MoveTemp(InWave);
		Language = MoveTemp(InLanguage);
	}
};

USTRUCT(BlueprintType, Category="HMI|SpeechToText")
struct HMI_API FHMISpeechToTextResult : public FHMIProcessorResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FString Text;

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FName Language;

	FHMISpeechToTextResult() = default;

	FHMISpeechToTextResult(double InTimestamp, bool InSuccess, FString InError, FString InText, FName InLanguage) : 
		FHMIProcessorResult(InTimestamp, InSuccess, MoveTemp(InError)),
		Text(MoveTemp(InText)),
		Language(MoveTemp(InLanguage))
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHMIOnSpeechToTextComplete, const FHMISpeechToTextInput&, Input, const FHMISpeechToTextResult&, Result);

UCLASS(BlueprintType, Abstract, ClassGroup="HMI|SpeechToText")
class HMI_API UHMISpeechToText : public UHMIProcessor
{
	GENERATED_BODY()

	public:

	UHMISpeechToText(const FObjectInitializer& ObjectInitializer);
	UHMISpeechToText(FVTableHelper& Helper);
	~UHMISpeechToText();

	UPROPERTY(BlueprintAssignable, Category="HMI|SpeechToText")
	FHMIOnSpeechToTextComplete OnSpeechToTextComplete;

	UFUNCTION(BlueprintCallable, Category="HMI|SpeechToText")
	virtual void SetDefaultLanguage(const FString& Value) { SetProcessorParam("DefaultLanguage", Value); }

	UFUNCTION(BlueprintCallable, Category="HMI|SpeechToText")
	virtual int64 ProcessWave(FName UserTag, FHMIWaveHandle Wave, FName Language)
	{ 
		return ProcessInput(FHMISpeechToTextInput(MoveTemp(UserTag), MoveTemp(Wave), MoveTemp(Language)));
	}

	virtual int64 ProcessInput(FHMISpeechToTextInput&& Input) { return 0; }

	protected:

	FString ModelName;
};
