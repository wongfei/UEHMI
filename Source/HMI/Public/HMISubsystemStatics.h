#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Processors/HMIProcessorType.h"
#include "HMISubsystemStatics.generated.h"

class UHMIProcessor;

UCLASS(ClassGroup="HMI|Subsystem")
class HMI_API UHMISubsystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
	
	// Default providers

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void SetDefaultProvider(UObject* WorldContextObject, EHMIProcessorType Type, FName Provider);

	UFUNCTION(BlueprintPure, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static FName GetDefaultProvider(UObject* WorldContextObject, EHMIProcessorType Type);

	// CreateProcessor

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static UHMIProcessor* CreateProcessor(UObject* WorldContextObject, FName Provider, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static UHMIProcessor* GetOrCreateProcessor(UObject* WorldContextObject, FName Provider, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static UHMIProcessor* CreateDefaultProcessor(UObject* WorldContextObject, EHMIProcessorType Type, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static UHMIProcessor* GetOrCreateDefaultProcessor(UObject* WorldContextObject, EHMIProcessorType Type, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static UHMIProcessor* GetExistingProcessor(UObject* WorldContextObject, FName Name, FName Provider = NAME_None);

	// CreateProcessor<T>

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static class UHMITextToSpeech* GetOrCreateTTS(UObject* WorldContextObject, FName Provider = NAME_None, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static class UHMISpeechToText* GetOrCreateSTT(UObject* WorldContextObject, FName Provider = NAME_None, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static class UHMIChat* GetOrCreateChat(UObject* WorldContextObject, FName Provider = NAME_None, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static class UHMILipSync* GetOrCreateLipSync(UObject* WorldContextObject, FName Provider = NAME_None, FName Name = NAME_None);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static class UHMIFaceDetector* GetOrCreateFaceDetector(UObject* WorldContextObject, FName Provider = NAME_None, FName Name = NAME_None);

	// Start/Stop all

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void StartAllProcessors(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void StopAllProcessors(UObject* WorldContextObject, bool WaitForCompletion = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void CancelAllProcessors(UObject* WorldContextObject, bool PurgeQueue = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void PauseAllProcessors(UObject* WorldContextObject, bool Cancel = false, bool PurgeQueue = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void ResumeAllProcessors(UObject* WorldContextObject);

	// Start/Stop chain

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void StartChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void StopChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain, bool WaitForCompletion = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void CancelChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain, bool PurgeQueue = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void PauseChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain, bool Cancel = false, bool PurgeQueue = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Subsystem", meta=(WorldContext="WorldContextObject"))
	static void ResumeChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain);
};
