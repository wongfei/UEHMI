#include "HMISubsystemStatics.h"
#include "HMISubsystem.h"

#define HMI_INST\
	auto* HMI = UHMISubsystem::GetInstance(WorldContextObject)

// Default providers

void UHMISubsystemStatics::SetDefaultProvider(UObject* WorldContextObject, EHMIProcessorType Type, FName Provider)
{
	HMI_INST;
	if (HMI)
		HMI->SetDefaultProvider(Type, Provider);
}

FName UHMISubsystemStatics::GetDefaultProvider(UObject* WorldContextObject, EHMIProcessorType Type)
{
	HMI_INST;
	return HMI ? HMI->GetDefaultProvider(Type) : NAME_None;
}

// CreateProcessor

UHMIProcessor* UHMISubsystemStatics::CreateProcessor(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->CreateProcessor(Provider, Name) : nullptr;
}

UHMIProcessor* UHMISubsystemStatics::GetOrCreateProcessor(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateProcessor(Provider, Name) : nullptr;
}

UHMIProcessor* UHMISubsystemStatics::CreateDefaultProcessor(UObject* WorldContextObject, EHMIProcessorType Type, FName Name)
{
	HMI_INST;
	return HMI ? HMI->CreateDefaultProcessor(Type, Name) : nullptr;
}

UHMIProcessor* UHMISubsystemStatics::GetOrCreateDefaultProcessor(UObject* WorldContextObject, EHMIProcessorType Type, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateDefaultProcessor(Type, Name) : nullptr;
}

UHMIProcessor* UHMISubsystemStatics::GetExistingProcessor(UObject* WorldContextObject, FName Name, FName Provider)
{
	HMI_INST;
	return HMI ? HMI->GetExistingProcessor(Name, Provider) : nullptr;
}

// CreateProcessor<T>

UHMITextToSpeech* UHMISubsystemStatics::GetOrCreateTTS(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateTTS(Provider, Name) : nullptr;
}

UHMISpeechToText* UHMISubsystemStatics::GetOrCreateSTT(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateSTT(Provider, Name) : nullptr;
}

UHMIChat* UHMISubsystemStatics::GetOrCreateChat(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateChat(Provider, Name) : nullptr;
}

UHMILipSync* UHMISubsystemStatics::GetOrCreateLipSync(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateLipSync(Provider, Name) : nullptr;
}

UHMIFaceDetector* UHMISubsystemStatics::GetOrCreateFaceDetector(UObject* WorldContextObject, FName Provider, FName Name)
{
	HMI_INST;
	return HMI ? HMI->GetOrCreateFaceDetector(Provider, Name) : nullptr;
}

// Start/Stop all

void UHMISubsystemStatics::StartAllProcessors(UObject* WorldContextObject)
{
	HMI_INST;
	if (HMI)
		HMI->StartAllProcessors();
}

void UHMISubsystemStatics::StopAllProcessors(UObject* WorldContextObject, bool WaitForCompletion)
{
	HMI_INST;
	if (HMI)
		HMI->StopAllProcessors(WaitForCompletion);
}

void UHMISubsystemStatics::CancelAllProcessors(UObject* WorldContextObject, bool PurgeQueue)
{
	HMI_INST;
	if (HMI)
		HMI->CancelAllProcessors(PurgeQueue);
}

void UHMISubsystemStatics::PauseAllProcessors(UObject* WorldContextObject, bool Cancel, bool PurgeQueue)
{
	HMI_INST;
	if (HMI)
		HMI->PauseAllProcessors(Cancel, PurgeQueue);
}

void UHMISubsystemStatics::ResumeAllProcessors(UObject* WorldContextObject)
{
	HMI_INST;
	if (HMI)
		HMI->ResumeAllProcessors();
}

//
// Start/Stop chain
//

void UHMISubsystemStatics::StartChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain)
{
	HMI_INST;
	if (HMI)
		HMI->StartChain(Chain);
}

void UHMISubsystemStatics::StopChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain, bool WaitForCompletion)
{
	HMI_INST;
	if (HMI)
		HMI->StopChain(Chain, WaitForCompletion);
}

void UHMISubsystemStatics::CancelChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain, bool PurgeQueue)
{
	HMI_INST;
	if (HMI)
		HMI->CancelChain(Chain, PurgeQueue);
}

void UHMISubsystemStatics::PauseChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain, bool Cancel, bool PurgeQueue)
{
	HMI_INST;
	if (HMI)
		HMI->PauseChain(Chain, Cancel, PurgeQueue);
}

void UHMISubsystemStatics::ResumeChain(UObject* WorldContextObject, const TArray<UHMIProcessor*>& Chain)
{
	HMI_INST;
	if (HMI)
		HMI->ResumeChain(Chain);
}

#undef HMI_INST
