#pragma once

#include "HMIMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HMIStatics.generated.h"

UCLASS(ClassGroup="HMI|Utils")
class HMI_API UHMIStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
	
	static int64 GenerateOperationId();
	static double GetTimestamp(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static int GetNumberOfCores();

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString GetPluginDir();

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString GetPluginBinDir();

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString GetPluginThirdPartyBinDir();

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString GetPluginDllFilepath(const FString& DllNameWithoutExt);

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString GetHMIDataDir();

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString LocateDataFile(const FString& Filename);

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString LocateDataSubdir(const FString& Dirname);

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString GetSecret(const FString& Name);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static TArray<FString> FindFiles(const FString& Dirpath, const FString& Pattern = TEXT("*.*"));

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static FString ReadStringFromFile(const FString& Filepath);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static TArray<FString> ReadLinesFromFile(const FString& Filepath);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static bool WriteStringToFile(const FString& Filepath, const FString& Content, bool Append = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static void RemoveSpecialCharacters(UPARAM(ref) FString& Input);

	UFUNCTION(BlueprintCallable, Category="HMI|Utils")
	static void NormalizeText(UPARAM(ref) FString& Input);

	UFUNCTION(BlueprintPure, Category="HMI|Utils")
	static FString RegexReplace(const FString& Input, const FString& Pattern, const FString& Replacement);
};
