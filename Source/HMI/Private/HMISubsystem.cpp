#include "HMISubsystem.h"

#include "Http/HMIDownloader.h"
#include "Audio/HMIResampler.h"

#include "Processors/HMITextToSpeech.h"
#include "Processors/HMISpeechToText.h"
#include "Processors/HMIChat.h"
#include "Processors/HMILipSync.h"
#include "Processors/HMIFaceDetector.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

static int32 HMIDebugProcessors = 0;
static FAutoConsoleVariableRef CVar_HMIDebugProcessors(TEXT("hmi.DebugProcessors"), HMIDebugProcessors, TEXT(""), ECVF_Default);

UHMISubsystem* UHMISubsystem::GetInstance(UObject* WorldContextObject) // static
{
	if (WorldContextObject)
	{
		UWorld* World = WorldContextObject->GetWorld();
		if (World)
		{
			UGameInstance* GameInst = World->GetGameInstance();
			if (GameInst)
			{
				return GameInst->GetSubsystem<UHMISubsystem>();
			}
		}
	}
	return nullptr;
}

UHMISubsystem::UHMISubsystem()
{
}

UHMISubsystem::~UHMISubsystem()
{
}

void UHMISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogHMI, Verbose, TEXT("UHMISubsystem::Initialize"));

	PrivData = MakeUnique<FPrivData>();

	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UHMISubsystem::OnPostWorldInitialization);
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UHMISubsystem::OnWorldBeginTearDown);

	#if !UE_BUILD_SHIPPING
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(TEXT("HMITicker"), 0.0f, [this](float DeltaTime)
	{
		TickSubsystem(DeltaTime);
		return true;
	});
	#endif

	UE_LOG(LogHMI, Verbose, TEXT("UHMISubsystem::Initialize DONE"));
}

void UHMISubsystem::Deinitialize()
{
	UE_LOG(LogHMI, Verbose, TEXT("UHMISubsystem::Deinitialize"));

	#if !UE_BUILD_SHIPPING
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	#endif

	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);

	StopAllProcessors(true);
	Processors.Reset();

	PrivData.Reset();

	UE_LOG(LogHMI, Verbose, TEXT("UHMISubsystem::Deinitialize DONE"));
}

UHMISubsystem::FPrivData::FPrivData()
{
	Downloader = MakeShared<FHMIDownloader>();

	Resampler = MakeUnique<FHMIResampler>();
	Resampler->InitDefaultVoice();
}

UHMISubsystem::FPrivData::~FPrivData()
{
}

//
// Tick
//

void UHMISubsystem::TickSubsystem(float DeltaTime)
{
	if (HMIDebugProcessors)
	{
		UEnum* StateEnum = StaticEnum<EHMIProcessorState>();
		FString State;

		for (auto& Iter : Processors)
		{
			State = StateEnum->GetNameStringByValue((int64)Iter.Value->GetProcessorState());
			float OpDuration = Iter.Value->GetLastOperationDuration();
			int32 QueueLen = Iter.Value->GetProcessorQueueLength();

			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(
				TEXT("%s # %s # %.1f # %d"), *Iter.Key.ToString(), *State, OpDuration, QueueLen
			));
		}
	}
}

//
// FWorldDelegates
//

void UHMISubsystem::OnPostWorldInitialization(UWorld* /*World*/, const UWorld::InitializationValues /*IVS*/)
{
	UE_LOG(LogHMI, Verbose, TEXT("OnPostWorldInitialization"));
}

void UHMISubsystem::OnWorldBeginTearDown(UWorld* /*World*/)
{
	UE_LOG(LogHMI, Verbose, TEXT("OnWorldBeginTearDown"));

	StopAllProcessors(true);
}

//
// Default providers
//

void UHMISubsystem::RegisterProvider(FName Provider, class UClass* ProviderClass)
{
	check(IsInGameThread());

	Providers.Add(Provider, ProviderClass);
}

void UHMISubsystem::SetDefaultProvider(EHMIProcessorType Type, FName Provider)
{
	check(IsInGameThread());

	DefaultProviders.Add(Type, Provider);
}

FName UHMISubsystem::GetDefaultProvider(EHMIProcessorType Type)
{
	auto Iter = DefaultProviders.Find(Type);
	return Iter ? *Iter : NAME_None;
}

//
// CreateProcessor
//

UHMIProcessor* UHMISubsystem::CreateProcessor(FName Provider, FName Name)
{
	check(IsInGameThread());

	UE_LOG(LogHMI, Verbose, TEXT("CreateProcessor: Provider=%s Name=%s"), *Provider.ToString(), *Name.ToString());

	if (Provider.IsNone())
	{
		UE_LOG(LogHMI, Error, TEXT("Provider name is empty"));
		return nullptr;
	}

	if (Name.IsNone())
	{
		Name = Provider;
	}

	auto ProcIter = Processors.Find(Name);
	if (ProcIter)
	{
		UE_LOG(LogHMI, Error, TEXT("Processor already exists: Provider=%s Name=%s"), *Provider.ToString(), *Name.ToString());
		return nullptr;
	}

	auto ProviderClassIter = Providers.Find(Provider);
	if (!ProviderClassIter)
	{
		UE_LOG(LogHMI, Error, TEXT("Provider not found: %s"), *Provider.ToString());
		return nullptr;
	}

	UHMIProcessor* Proc = NewObject<UHMIProcessor>(this, *ProviderClassIter);
	if (!Proc)
	{
		UE_LOG(LogHMI, Error, TEXT("Failed to create processor: Provider=%s Name=%s"), *Provider.ToString(), *Name.ToString());
		return nullptr;
	}

	Proc->ProviderName = Provider;
	Proc->ProcessorName = Name.ToString();

	Proc->HMISubsystem = this;
	Proc->GameInstance = GetGameInstance();

	Processors.Add(Name, Proc);

	Proc->OnProcessorCreated();
	OnProcessorCreated.Broadcast(Proc);

	return Proc;
}

UHMIProcessor* UHMISubsystem::GetOrCreateProcessor(FName Provider, FName Name)
{
	check(IsInGameThread());

	if (Name.IsNone())
		Name = Provider;

	auto ProcIter = Processors.Find(Name);
	if (ProcIter)
		return *ProcIter;

	return CreateProcessor(Provider, Name);
}

UHMIProcessor* UHMISubsystem::CreateDefaultProcessor(EHMIProcessorType Type, FName Name)
{
	FName Provider = GetDefaultProvider(Type);
	return CreateProcessor(Provider, Name);
}

UHMIProcessor* UHMISubsystem::GetOrCreateDefaultProcessor(EHMIProcessorType Type, FName Name)
{
	FName Provider = GetDefaultProvider(Type);
	return GetOrCreateProcessor(Provider, Name);
}

UHMIProcessor* UHMISubsystem::GetExistingProcessor(FName Name, FName Provider)
{
	check(IsInGameThread());

	if (Provider.IsNone())
	{
		auto Iter = Processors.Find(Name);
		return Iter ? *Iter : nullptr;
	}

	for (const auto& Iter : Processors)
	{
		if (Iter.Value->ProviderName == Provider && (Name.IsNone() || Iter.Key == Name))
		{
			return Iter.Value.Get();
		}
	}

	return nullptr;
}

// CreateProcessor<T>

UHMITextToSpeech* UHMISubsystem::GetOrCreateTTS(FName Provider, FName Name)
{
	if (Provider.IsNone() && Name.IsNone()) 
		Provider = GetDefaultProvider(EHMIProcessorType::TTS);
	return Cast<UHMITextToSpeech>(GetOrCreateProcessor(Provider, Name));
}

UHMISpeechToText* UHMISubsystem::GetOrCreateSTT(FName Provider, FName Name)
{
	if (Provider.IsNone() && Name.IsNone()) 
		Provider = GetDefaultProvider(EHMIProcessorType::STT);
	return Cast<UHMISpeechToText>(GetOrCreateProcessor(Provider, Name));
}

UHMIChat* UHMISubsystem::GetOrCreateChat(FName Provider, FName Name)
{
	if (Provider.IsNone() && Name.IsNone()) 
		Provider = GetDefaultProvider(EHMIProcessorType::Chat);
	return Cast<UHMIChat>(GetOrCreateProcessor(Provider, Name));
}

UHMILipSync* UHMISubsystem::GetOrCreateLipSync(FName Provider, FName Name)
{
	if (Provider.IsNone() && Name.IsNone()) 
		Provider = GetDefaultProvider(EHMIProcessorType::LipSync);
	return Cast<UHMILipSync>(GetOrCreateProcessor(Provider, Name));
}

UHMIFaceDetector* UHMISubsystem::GetOrCreateFaceDetector(FName Provider, FName Name)
{
	if (Provider.IsNone() && Name.IsNone()) 
		Provider = GetDefaultProvider(EHMIProcessorType::FaceDetector);
	return Cast<UHMIFaceDetector>(GetOrCreateProcessor(Provider, Name));
}

//
// Start/Stop all
//

void UHMISubsystem::StartAllProcessors()
{
	check(IsInGameThread());

	for (auto& Iter : Processors)
	{
		Iter.Value->StartOrWakeProcessor();
	}
}

void UHMISubsystem::StopAllProcessors(bool WaitForCompletion)
{
	UE_LOG(LogHMI, Verbose, TEXT("StopAllProcessors"));

	check(IsInGameThread());

	for (auto& Iter : Processors)
	{
		Iter.Value->StopProcessor(false);
	}

	// 2nd pass with wait
	if (WaitForCompletion)
	{
		for (auto& Iter : Processors)
		{
			Iter.Value->StopProcessor(true);
		}
	}

	UE_LOG(LogHMI, Verbose, TEXT("StopAllProcessors DONE"));
}

void UHMISubsystem::CancelAllProcessors(bool PurgeQueue)
{
	check(IsInGameThread());

	for (auto& Iter : Processors)
	{
		Iter.Value->CancelOperation(PurgeQueue);
	}
}

void UHMISubsystem::PauseAllProcessors(bool Cancel, bool PurgeQueue)
{
	check(IsInGameThread());

	for (auto& Iter : Processors)
	{
		Iter.Value->PauseProcessor(Cancel, PurgeQueue);
	}
}

void UHMISubsystem::ResumeAllProcessors()
{
	check(IsInGameThread());

	for (auto& Iter : Processors)
	{
		Iter.Value->ResumeProcessor();
	}
}

//
// Start/Stop chain
//

void UHMISubsystem::StartChain(const TArray<UHMIProcessor*>& Chain)
{
	check(IsInGameThread());

	for (UHMIProcessor* Proc : Chain)
	{
		if (Proc)
			Proc->StartOrWakeProcessor();
	}
}

void UHMISubsystem::StopChain(const TArray<UHMIProcessor*>& Chain, bool WaitForCompletion)
{
	check(IsInGameThread());

	for (UHMIProcessor* Proc : Chain)
	{
		if (Proc)
			Proc->StopProcessor(false);
	}

	// 2nd pass with wait
	if (WaitForCompletion)
	{
		for (UHMIProcessor* Proc : Chain)
		{
			if (Proc)
				Proc->StopProcessor(true);
		}
	}
}

void UHMISubsystem::CancelChain(const TArray<UHMIProcessor*>& Chain, bool PurgeQueue)
{
	check(IsInGameThread());

	for (UHMIProcessor* Proc : Chain)
	{
		if (Proc)
			Proc->CancelOperation(PurgeQueue);
	}
}

void UHMISubsystem::PauseChain(const TArray<UHMIProcessor*>& Chain, bool Cancel, bool PurgeQueue)
{
	check(IsInGameThread());

	for (UHMIProcessor* Proc : Chain)
	{
		if (Proc)
			Proc->PauseProcessor(Cancel, PurgeQueue);
	}
}

void UHMISubsystem::ResumeChain(const TArray<UHMIProcessor*>& Chain)
{
	check(IsInGameThread());

	for (UHMIProcessor* Proc : Chain)
	{
		if (Proc)
			Proc->ResumeProcessor();
	}
}

//
// Misc
//

FHMIWaveHandle UHMISubsystem::Resample(const FHMIWaveHandle& Wave, int OutputSampleRate)
{
	TArrayView<const int16> View(Wave);
	FHMIWaveHandle OutWave = FHMIWaveHandle::Alloc_PCM_S16(OutputSampleRate, Wave.GetNumChannels());

	{
		FScopeLock LOCK(&PrivData->ResamplerMux);
		PrivData->Resampler->Convert(View, Wave.GetSampleRate(), PrivData->ResampledS16, OutputSampleRate);
		OutWave.CopyFrom(PrivData->ResampledS16);
	}

	return OutWave;
}
