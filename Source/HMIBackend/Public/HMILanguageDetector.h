#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "HMILanguageDetector.generated.h"

UCLASS()
class HMIBACKEND_API UHMILanguageDetector : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
	
	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FName DetectLanguage(const FString& Text);
};
