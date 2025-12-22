#include "Components/HMILipSyncComponent.h"
#include "Components/HMIPoseRemapComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

#define LOGPREFIX "[LipSyncComp] "

UHMILipSyncComponent::UHMILipSyncComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UHMILipSyncComponent::UHMILipSyncComponent(FVTableHelper& Helper) : Super(Helper)
{
}

UHMILipSyncComponent::~UHMILipSyncComponent()
{
}

void UHMILipSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Actor = GetOwner();
	check(Actor);

	RemapComp = Actor->FindComponentByClass<UHMIPoseRemapComponent>();
	if (!RemapComp)
	{
		UE_LOG(LogHMI, Warning, TEXT(LOGPREFIX "HMIPoseRemapComponent not found"));
	}

	UHMIWavePlayerComponent* Player = Actor->FindComponentByClass<UHMIWavePlayerComponent>();
	if (!Player)
	{
		UE_LOG(LogHMI, Warning, TEXT(LOGPREFIX "HMIWavePlayerComponent not found"));
	}
	else
	{
		SetWavePlayer(Player);
	}
}

void UHMILipSyncComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetWavePlayer(nullptr);

	Super::EndPlay(EndPlayReason);
}

void UHMILipSyncComponent::SetWavePlayer(UHMIWavePlayerComponent* InPlayer)
{
	//UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "SetWavePlayer %s"), InPlayer ? *InPlayer->GetName() : TEXT("nullptr"));

	if (WavePlayer && WavePlayer != InPlayer)
	{
		//WavePlayer->OnWaveStart.RemoveAll(this);
		//WavePlayer->OnWaveStop.RemoveAll(this);
		WavePlayer->OnWaveProgress.RemoveAll(this);
		WavePlayer->OnPlayerUnderflow.RemoveAll(this);
		WavePlayer->OnPlayerStop.RemoveAll(this);
	}

	if (InPlayer)
	{
		//InPlayer->OnWaveStart.AddDynamic(this, &UHMILipSyncComponent::OnWaveStart);
		//InPlayer->OnWaveStop.AddDynamic(this, &UHMILipSyncComponent::OnWaveStop);
		InPlayer->OnWaveProgress.AddDynamic(this, &UHMILipSyncComponent::OnWaveProgress);
		InPlayer->OnPlayerUnderflow.AddDynamic(this, &UHMILipSyncComponent::OnPlayerUnderflow);
		InPlayer->OnPlayerStop.AddDynamic(this, &UHMILipSyncComponent::OnPlayerStop);
	}

	WavePlayer = InPlayer;
	ResetLipSync();
}

void UHMILipSyncComponent::OnWaveStart(const FHMIPlayWaveInput& Input)
{
}

void UHMILipSyncComponent::OnWaveStop(const FHMIPlayWaveInput& Input, const FHMIPlayWaveResult& Result)
{
}

void UHMILipSyncComponent::OnWaveProgress(bool WasStarted, float ProgressSeconds, float DurationSeconds)
{
	if (WasStarted) // begin play
	{
		Sequence = WavePlayer->GetCurrentSequence();

		if (Sequence.IsValid())
		{
			UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) <<< Duration=%.4f NumFrames=%d"), 
				GetWorld()->GetAudioTimeSeconds(), DurationSeconds, Sequence.GetFrames().Num());
		}
	}

	if (Sequence.IsValid())
	{
		if (ProgressSeconds + TrimLastSeconds <= DurationSeconds)
		{
			const int NewFrameId = (int)(ProgressSeconds * Sequence.GetFrameRate());
			if ((NewFrameId == 0 || NewFrameId != FrameId) && NewFrameId >= 0 && NewFrameId < Sequence.GetFrames().Num())
			{
				Frame = Sequence.GetFrames()[NewFrameId];
				FrameId = NewFrameId;
				SubmitFrame();
			}
		}
		else // end of sequence
		{
			ResetLipSync();
		}
	}
}

void UHMILipSyncComponent::OnPlayerUnderflow()
{
	ResetLipSync();
}

void UHMILipSyncComponent::OnPlayerStop()
{
	ResetLipSync();
}

void UHMILipSyncComponent::ResetLipSync()
{
	if (Sequence.IsValid())
	{
		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) ResetLipSync"), GetWorld()->GetAudioTimeSeconds());
		Sequence.Reset();
	}

	{
		FMemory::Memzero(Frame.Weights.GetData(), Frame.Weights.Num() * sizeof(Frame.Weights[0]));
		Frame.LaughterScore = 0;
		FrameId = 0;
		SubmitFrame();
	}
}

void UHMILipSyncComponent::SubmitFrame()
{
	if (!RemapComp)
		return;

	const int NumWeights = Frame.Weights.Num();
	for (int Id = 0; Id < NumWeights; ++Id)
	{
		FName Name = Frame.GetWeightName(Id);
		if (Name != NAME_None)
		{
			RemapComp->SetInput(Name, Frame.Weights[Id]);
		}
	}

	RemapComp->SubmitUpdate();
}

#undef LOGPREFIX
