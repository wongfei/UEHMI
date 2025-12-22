#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "Containers/Ticker.h"
#include "Processors/HMIProcessorType.h"
#include "HMIBuffer.h"
#include "HMISubsystem.generated.h"

class UHMIProcessor;

DECLARE_MULTICAST_DELEGATE_OneParam(FHMIOnProcessorCreated, UHMIProcessor*);

UCLASS(ClassGroup="HMI|Subsystem")
class HMI_API UHMISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	friend class UHMIBackendSubsystem;

	public:

	static UHMISubsystem* GetInstance(UObject* WorldContextObject);

	UHMISubsystem();
	~UHMISubsystem();

	void TickSubsystem(float DeltaTime);

	// USubsystem

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FWorldDelegates

	void OnPostWorldInitialization(UWorld* /*World*/, const UWorld::InitializationValues /*IVS*/);
	void OnWorldBeginTearDown(UWorld* /*World*/);

	// Default providers

	void RegisterProvider(FName Provider, class UClass* ProviderClass);
	void SetDefaultProvider(EHMIProcessorType Type, FName Provider);
	FName GetDefaultProvider(EHMIProcessorType Type);

	// CreateProcessor

	UHMIProcessor* CreateProcessor(FName Provider, FName Name = NAME_None);
	UHMIProcessor* GetOrCreateProcessor(FName Provider, FName Name = NAME_None);
	UHMIProcessor* CreateDefaultProcessor(EHMIProcessorType Type, FName Name = NAME_None);
	UHMIProcessor* GetOrCreateDefaultProcessor(EHMIProcessorType Type, FName Name = NAME_None);
	UHMIProcessor* GetExistingProcessor(FName Name, FName Provider = NAME_None);

	FHMIOnProcessorCreated OnProcessorCreated;

	// CreateProcessor<T>

	class UHMITextToSpeech* GetOrCreateTTS(FName Provider = NAME_None, FName Name = NAME_None);
	class UHMISpeechToText* GetOrCreateSTT(FName Provider = NAME_None, FName Name = NAME_None);
	class UHMIChat* GetOrCreateChat(FName Provider = NAME_None, FName Name = NAME_None);
	class UHMILipSync* GetOrCreateLipSync(FName Provider = NAME_None, FName Name = NAME_None);
	class UHMIFaceDetector* GetOrCreateFaceDetector(FName Provider = NAME_None, FName Name = NAME_None);

	// Start/Stop

	void StartAllProcessors();
	void StopAllProcessors(bool WaitForCompletion = false);
	void CancelAllProcessors(bool PurgeQueue = false);
	void PauseAllProcessors(bool Cancel = false, bool PurgeQueue = false);
	void ResumeAllProcessors();

	void StartChain(const TArray<UHMIProcessor*>& Chain);
	void StopChain(const TArray<UHMIProcessor*>& Chain, bool WaitForCompletion = false);
	void CancelChain(const TArray<UHMIProcessor*>& Chain, bool PurgeQueue = false);
	void PauseChain(const TArray<UHMIProcessor*>& Chain, bool Cancel = false, bool PurgeQueue = false);
	void ResumeChain(const TArray<UHMIProcessor*>& Chain);

	// Misc

	TSharedPtr<class FHMIDownloader> GetDownloader() { return PrivData->Downloader; }
	FHMIWaveHandle Resample(const FHMIWaveHandle& Wave, int OutputSampleRate); // blocking!

	protected:

	struct FPrivData
	{
		FPrivData();
		~FPrivData();

		TSharedPtr<class FHMIDownloader> Downloader;

		mutable FCriticalSection ResamplerMux;
		TUniquePtr<class FHMIResampler> Resampler;
		TArray<int16> ResampledS16;
	};

	TUniquePtr<FPrivData> PrivData;

	UPROPERTY(Transient, BlueprintReadOnly, Category="HMI|Subsystem")
	TMap<FName, UClass*> Providers;

	UPROPERTY(Transient, BlueprintReadOnly, Category="HMI|Subsystem")
	TMap<EHMIProcessorType, FName> DefaultProviders;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UHMIProcessor>> Processors;

	FTSTicker::FDelegateHandle TickHandle;
};
