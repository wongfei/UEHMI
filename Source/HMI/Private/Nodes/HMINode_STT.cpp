#include "HMINode_STT.h"
#include "HMISubsystem.h"

UHMINode_STT* UHMINode_STT::STT_Async(UObject* WorldContextObject, UHMIProcessor* Processor, FName UserTag, FHMIWaveHandle Wave, FName Language)
{
	UHMINode_STT* Node = NewObject<UHMINode_STT>();
	Node->WorldContext = WorldContextObject;
	Node->Processor = Cast<UHMISpeechToText>(Processor);
	Node->UserTag = MoveTemp(UserTag);
	Node->Wave = MoveTemp(Wave);
	Node->Language = MoveTemp(Language);
	return Node;
}

void UHMINode_STT::Activate()
{
	if (Complete.IsBound())
	{
		UHMISubsystem* HMI = UHMISubsystem::GetInstance(WorldContext);
		if (ensure(HMI))
		{
			if (!Processor)
				Processor = HMI->GetOrCreateSTT();

			if (ensure(Processor))
			{
				FHMISpeechToTextInput Input(MoveTemp(UserTag), MoveTemp(Wave), MoveTemp(Language));

				Input.CompleteFunc = [this](FHMIProcessorInput& ProcInput, FHMIProcessorResult& ProcResult)
				{
					auto& Input = static_cast<FHMISpeechToTextInput&>(ProcInput);
					auto& Result = static_cast<FHMISpeechToTextResult&>(ProcResult);

					AsyncTask(ENamedThreads::GameThread, [this, Input = MoveTemp(Input), Result = MoveTemp(Result)] () mutable
					{
						Complete.Broadcast(Input, Result);
						SetReadyToDestroy();
					});
				};

				RegisterWithGameInstance(WorldContext);
				Processor->ProcessInput(MoveTemp(Input));
			}
		}
	}
}
