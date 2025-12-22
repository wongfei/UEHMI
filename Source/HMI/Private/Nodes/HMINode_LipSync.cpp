#include "HMINode_LipSync.h"
#include "HMISubsystem.h"

UHMINode_LipSync* UHMINode_LipSync::LipSync_Async(UObject* WorldContextObject, UHMIProcessor* Processor, FName UserTag, FHMIWaveHandle Wave)
{
	UHMINode_LipSync* Node = NewObject<UHMINode_LipSync>();
	Node->WorldContext = WorldContextObject;
	Node->Processor = Cast<UHMILipSync>(Processor);
	Node->UserTag = MoveTemp(UserTag);
	Node->Wave = MoveTemp(Wave);
	return Node;
}

void UHMINode_LipSync::Activate()
{
	if (Complete.IsBound())
	{
		UHMISubsystem* HMI = UHMISubsystem::GetInstance(WorldContext);
		if (ensure(HMI))
		{
			if (!Processor)
				Processor = HMI->GetOrCreateLipSync();

			if (ensure(Processor))
			{
				FHMILipSyncInput Input(MoveTemp(UserTag), MoveTemp(Wave));

				Input.CompleteFunc = [this](FHMIProcessorInput& ProcInput, FHMIProcessorResult& ProcResult)
				{
					auto& Input = static_cast<FHMILipSyncInput&>(ProcInput);
					auto& Result = static_cast<FHMILipSyncResult&>(ProcResult);

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
