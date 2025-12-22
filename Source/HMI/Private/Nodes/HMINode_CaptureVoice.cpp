#include "HMINode_CaptureVoice.h"
#include "HMISubsystem.h"

UHMINode_CaptureVoice* UHMINode_CaptureVoice::CaptureVoice_Async(UObject* WorldContextObject, UHMIVoiceCaptureComponent* Capture, FName UserTag)
{
	UHMINode_CaptureVoice* Node = NewObject<UHMINode_CaptureVoice>();
	Node->WorldContext = WorldContextObject;
	Node->Capture = Capture;
	Node->UserTag = MoveTemp(UserTag);
	return Node;
}

void UHMINode_CaptureVoice::Activate()
{
	if (Complete.IsBound() && IsValid(Capture))
	{
		Capture->OnSentenceCaptured.AddDynamic(this, &UHMINode_CaptureVoice::OnSentenceCaptured);
		Capture->StartCapture();

		RegisterWithGameInstance(WorldContext);
		RegisteredFlag = true;
	}
}

void UHMINode_CaptureVoice::OnSentenceCaptured(FHMIWaveHandle Wave)
{
	if (RegisteredFlag)
	{
		RegisteredFlag = false;
		if (IsValid(Capture))
		{
			Capture->OnSentenceCaptured.RemoveAll(this);
		}
		Complete.Broadcast(MoveTemp(Wave));
		SetReadyToDestroy();
	}
}
