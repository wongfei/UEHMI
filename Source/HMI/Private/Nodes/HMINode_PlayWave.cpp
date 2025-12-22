#include "HMINode_PlayWave.h"
#include "HMISubsystem.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

UHMINode_PlayWave* UHMINode_PlayWave::PlayWave_Async(UObject* WorldContextObject, UHMIWavePlayerComponent* Player, FName UserTag, FHMIWaveHandle Wave, FHMILipSyncSequenceHandle Sequence)
{
	UHMINode_PlayWave* Node = NewObject<UHMINode_PlayWave>();
	Node->WorldContext = WorldContextObject;
	Node->Player = Player;
	Node->UserTag = MoveTemp(UserTag);
	Node->Wave = MoveTemp(Wave);
	Node->Sequence = MoveTemp(Sequence);
	return Node;
}

void UHMINode_PlayWave::Activate()
{
	if (Complete.IsBound() && IsValid(Player))
	{
		FHMIPlayWaveInput Input(MoveTemp(UserTag), MoveTemp(Wave), MoveTemp(Sequence));

		Input.CompleteFunc = [this](FHMIProcessorInput& ProcInput, FHMIProcessorResult& ProcResult)
		{
			auto& Input = static_cast<FHMIPlayWaveInput&>(ProcInput);
			auto& Result = static_cast<FHMIPlayWaveResult&>(ProcResult);

			AsyncTask(ENamedThreads::GameThread, [this, Input = MoveTemp(Input), Result = MoveTemp(Result)] () mutable
			{
				Complete.Broadcast(Input, Result);
				SetReadyToDestroy();
			});
		};

		RegisterWithGameInstance(WorldContext);
		Player->ProcessInput(MoveTemp(Input));
	}
}
