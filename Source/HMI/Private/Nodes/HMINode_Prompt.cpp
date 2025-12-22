#include "HMINode_Prompt.h"
#include "HMISubsystem.h"

UHMINode_Prompt* UHMINode_Prompt::Prompt_Async(UObject* WorldContextObject, UHMIProcessor* Processor, 
	FName UserTag, FString Text, 
	TArray<FHMIChatMessage> History, TMap<FString, FString> BackendParams, 
	int MaxTokens, float Temperature, float TopP, bool Streaming
)
{
	UHMINode_Prompt* Node = NewObject<UHMINode_Prompt>();
	Node->WorldContext = WorldContextObject;
	Node->Processor = Cast<UHMIChat>(Processor);
	Node->UserTag = MoveTemp(UserTag);
	Node->Text = MoveTemp(Text);
	Node->History = MoveTemp(History);
	Node->BackendParams = MoveTemp(BackendParams);
	Node->MaxTokens = MaxTokens;
	Node->Temperature = Temperature;
	Node->TopP = TopP;
	Node->Streaming = Streaming;
	return Node;
}

void UHMINode_Prompt::Activate()
{
	if (Progress.IsBound())
	{
		UHMISubsystem* HMI = UHMISubsystem::GetInstance(WorldContext);
		if (ensure(HMI))
		{
			if (!Processor)
				Processor = HMI->GetOrCreateChat();

			if (ensure(Processor))
			{
				FHMIChatInput Input(MoveTemp(UserTag), MoveTemp(Text), MoveTemp(History), MoveTemp(BackendParams), MaxTokens, Temperature, TopP);

				if (Streaming)
				{
					Input.ChatBatchFunc = [this](const FName& UserTag, const FString& Batch, bool EndOfText)
					{
						AsyncTask(ENamedThreads::GameThread, [this, UserTag = FName(UserTag), Batch = FString(Batch), EndOfText]()
						{
							Progress.Broadcast(UserTag, Batch, EndOfText);
							if (EndOfText) 
								SetReadyToDestroy();
						});
					};
				}
				else
				{
					Input.CompleteFunc = [this](FHMIProcessorInput& ProcInput, FHMIProcessorResult& ProcResult)
					{
						auto& Input = static_cast<FHMIChatInput&>(ProcInput);
						auto& Result = static_cast<FHMIChatResult&>(ProcResult);

						AsyncTask(ENamedThreads::GameThread, [this, Input = MoveTemp(Input), Result = MoveTemp(Result)]() mutable
						{
							Progress.Broadcast(UserTag, Result.Text, /*EndOfText*/true);
							SetReadyToDestroy();
						});
					};
				}

				RegisterWithGameInstance(WorldContext);
				Processor->ProcessInput(MoveTemp(Input));
			}
		}
	}
}
