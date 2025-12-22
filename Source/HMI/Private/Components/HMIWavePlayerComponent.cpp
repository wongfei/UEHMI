#include "Components/HMIWavePlayerComponent.h"
#include "Audio/HMIResampler.h"

#include "AudioDevice.h"
#include "DSP/FloatArrayMath.h"
#include "Engine/Engine.h"

DECLARE_CYCLE_STAT(TEXT("WavePlayer_Tick"), STAT_HMI_WavePlayer_Tick, STATGROUP_HMI)
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("WavePlayer_PlayNextBuffer"), STAT_HMI_WavePlayer_PlayNextBuffer, STATGROUP_HMI)

#define LOGPREFIX "[WavePlayer] "

UHMIWavePlayerComponent::UHMIWavePlayerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

UHMIWavePlayerComponent::UHMIWavePlayerComponent(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIWavePlayerComponent::~UHMIWavePlayerComponent()
{
}

void UHMIWavePlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	CreateAudioComponent();
}

void UHMIWavePlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAudioComponent();

	if (DebugBuffer.Num() > 0)
	{
		auto Wave = FHMIWaveHandle::Alloc_PCM_S16(OutputSampleRate, HMI_VOICE_CHANNELS);
		Wave.CopyFrom(DebugBuffer);

		UHMIBufferStatics::SaveWaveBuffer(Wave, UHMIStatics::LocateDataFile(TEXT("DebugBuffer.wav")));
		DebugBuffer.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void UHMIWavePlayerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_HMI_WavePlayer_Tick)

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TickPlaybackProgress(DeltaTime);
}

void UHMIWavePlayerComponent::CreateAudioComponent()
{
	if (AudioComponent)
		return;

	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "CreateComponent"));

	if (OutputSampleRate == 0)
	{
		OutputSampleRate = HMI_DEFAULT_SAMPLE_RATE;
	}

	if (!UHMIBufferStatics::IsValidSampleRate(OutputSampleRate))
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Invalid sample rate: %d"), OutputSampleRate);
		return;
	}

	check(GEngine);
	FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice();
	if (!AudioDevice)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "GetMainAudioDevice failed"));
		return;
	}

	ProceduralWave = NewObject<USoundWaveProcedural>();
	if (!ProceduralWave)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "NewObject USoundWaveProcedural failed"));
		return;
	}

	ProceduralWave->SetSampleRate(OutputSampleRate);
	ProceduralWave->NumChannels = HMI_VOICE_CHANNELS;
	ProceduralWave->SoundGroup = SOUNDGROUP_Voice;
	ProceduralWave->Duration = INDEFINITELY_LOOPING_DURATION;
	ProceduralWave->bLooping = false;
	ProceduralWave->bProcedural = true;
	ProceduralWave->bCanProcessAsync = true;

	ProceduralWave->SoundClassObject = SoundClass;
	ProceduralWave->AttenuationSettings = AttenuationSettings;
	ProceduralWave->SourceEffectChain = SourceEffectChain;

	ProceduralWave->bEnableBaseSubmix = bEnableBaseSubmix;
	ProceduralWave->SoundSubmixObject = SoundSubmix;

	ProceduralWave->bEnableSubmixSends = bEnableSubmixSends;
	ProceduralWave->SoundSubmixSends = SoundSubmixSends;

	ProceduralWave->bEnableBusSends = bEnableBusSends;
	ProceduralWave->PreEffectBusSends = PreEffectBusSends;
	ProceduralWave->BusSends = PostEffectBusSends;

	FAudioDevice::FCreateComponentParams Params(GetWorld(), GetOwner());
	AudioComponent = AudioDevice->CreateComponent(ProceduralWave, Params);
	if (!AudioComponent)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "FAudioDevice::CreateComponent failed"));
		return;
	}

	AudioComponent->Mobility = EComponentMobility::Movable;
	AudioComponent->bStopWhenOwnerDestroyed = true;
	AudioComponent->bShouldRemainActiveIfDropped = true;

	AudioComponent->bIsUISound = false;
	AudioComponent->bAllowSpatialization = true;
	AudioComponent->SetVolumeMultiplier(VolumeMultiplier);

	AudioComponent->SoundClassOverride = SoundClass;
	AudioComponent->AttenuationSettings = AttenuationSettings;
	AudioComponent->SourceEffectChain = SourceEffectChain;

	AudioComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
	AudioComponent->RegisterComponent();

	AudioComponent->OnAudioPlayStateChangedNative.AddUObject(this, &UHMIWavePlayerComponent::OnAudioPlayStateChanged);
	AudioComponent->OnAudioPlaybackPercentNative.AddUObject(this, &UHMIWavePlayerComponent::OnAudioPlaybackPercent);
	AudioComponent->OnAudioFinishedNative.AddUObject(this, &UHMIWavePlayerComponent::OnAudioPlaybackFinished);

	Resampler = MakeUnique<FHMIResampler>();
	Resampler->InitDefaultVoice();
}

void UHMIWavePlayerComponent::DestroyAudioComponent()
{
	if (AudioComponent)
	{
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "DestroyComponent"));

		AudioComponent->OnAudioPlayStateChangedNative.RemoveAll(this);
		AudioComponent->OnAudioPlaybackPercentNative.RemoveAll(this);
		AudioComponent->OnAudioFinishedNative.RemoveAll(this);

		AudioComponent->Stop();
		AudioComponent->DestroyComponent();
		AudioComponent = nullptr;
	}

	ProceduralWave = nullptr;
	Resampler.Reset();
}

int64 UHMIWavePlayerComponent::ProcessInput(FHMIPlayWaveInput&& Input)
{
	//UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) ProcessInput"), PlaybackTS);
	const int64 OperationId = EnqueOperation(InputQueue, MoveTemp(Input), this);
	PlayNextBuffer();
	return OperationId;
}

void UHMIWavePlayerComponent::PlayNextBuffer()
{
	if (!AudioComponent) 
		return;

	if (PrefetchTS > 0.0) // waiting for prefetch
		return;

	const uint64 CyclesStart = FPlatformTime::Cycles64();

	FHMIPlayWaveOperation Operation;
	int QueueLength = 0;

	if (!DequeOperation(InputQueue, Operation, QueueLength))
		return;

	const auto& Wave = Operation.Input.Wave;

	if (Wave.IsEmpty())
	{
		// silently ignore empty input
		HandleOperationComplete(Operation.Input, false);
		return;
	}

	if (!UHMIBufferStatics::IsValidVoice(Wave))
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Unsupported wave format"));
		HandleOperationComplete(Operation.Input, false);
		return;
	}

	if (Wave.GetDuration() <= FadeDurationSec * 2.0f)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Wave is too short"));
		HandleOperationComplete(Operation.Input, false);
		return;
	}

	PlaybackTS = GetAudioTS();
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) <<< Duration=%.4f"), PlaybackTS, Wave.GetDuration());

	const int WaveSampleRate = Wave.GetSampleRate();
	const float WaveDuration = Wave.GetDuration();
	TArrayView<const int16> SamplesView = Wave;

	// bad case, resampling on game thread!
	if (WaveSampleRate != OutputSampleRate)
	{
		const uint64 ResampleStart = FPlatformTime::Cycles64();

		Resampler->Convert(SamplesView, WaveSampleRate, PcmS16, OutputSampleRate);
		SamplesView = PcmS16;

		const uint64 ResampleEnd = FPlatformTime::Cycles64();
		const float ResampleElapsed = (float)FPlatformTime::ToMilliseconds64(ResampleEnd - ResampleStart);

		if (ResampleElapsed > 1.0f)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("SLOW RESAMPLING %.4f ms"), ResampleElapsed));
		}
	}

	const int NumSamples = SamplesView.Num();
	const int FadeSamples = (int)(FadeDurationSec * OutputSampleRate);

	PlaybackTS = GetAudioTS();
	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) QueueAudio: Duration=%f"), PlaybackTS, WaveDuration);

	if (FadeSamples > 0)
		Fadein(SamplesView);

	QueueSamples(SamplesView.GetData() + FadeSamples, NumSamples - FadeSamples * 2);

	if (FadeSamples > 0)
		Fadeout(SamplesView);

	if (UnderflowTS < PlaybackTS) // not playing now
		UnderflowTS = PlaybackTS;

	Operation.StartPlayTS = UnderflowTS;
	Operation.FinishPlayTS = UnderflowTS + WaveDuration;

	UnderflowTS = Operation.FinishPlayTS;
	AutostopTS = UnderflowTS + AutostopAfterInactiveSec;

	float PrefetchOff = FMath::Min(FadeDurationSec * 2.0f, WaveDuration * 0.5f);
	PrefetchOff = FMath::Max(PrefetchOff, MinPrefetchSec);
	PrefetchTS = UnderflowTS - PrefetchOff;

	//UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) WaveStartPlayTS=%f PrefetchTS=%f AutostopTS=%f"), PlaybackTS, Operation.StartPlayTS, PrefetchTS, AutostopTS);

	PendingOperations.Add(MoveTemp(Operation));

	if (!IsPlaying())
	{
		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) Play"), PlaybackTS);
		AudioComponent->Play();
	}

	const uint64 CyclesEnd = FPlatformTime::Cycles64();
	const float Elapsed = (float)FPlatformTime::ToMilliseconds64(CyclesEnd - CyclesStart);
	SET_FLOAT_STAT(STAT_HMI_WavePlayer_PlayNextBuffer, Elapsed)
}

void UHMIWavePlayerComponent::QueueBytes(const uint8* Data, int NumBytes)
{
	const float Duration = (NumBytes / sizeof(int16)) / (float)OutputSampleRate;
	ProceduralWave->QueueAudio(Data, NumBytes);

	if (EnableDebugBuffer)
	{
		DebugBuffer.Append(Data, NumBytes);
	}
}

void UHMIWavePlayerComponent::QueueSamples(const int16* Data, int NumSamples)
{
	QueueBytes((const uint8*)Data, NumSamples * sizeof(int16));
}

void UHMIWavePlayerComponent::Fadein(const TArrayView<const int16>& Wave)
{
	const int FadeSamples = (int)(FadeDurationSec * OutputSampleRate);
	const float InvFadeSamples = 1.0f / (float)FadeSamples;

	FadeBuffer.SetNumUninitialized(FadeSamples, EAllowShrinking::No);
	int16* Dst = FadeBuffer.GetData();
	const int16* Src = Wave.GetData();

	for (int i = 0; i < FadeSamples; ++i)
	{
		Dst[i] = (int16)(Src[i] * (i * InvFadeSamples));
	}

	QueueSamples(FadeBuffer.GetData(), FadeBuffer.Num());
}

void UHMIWavePlayerComponent::Fadeout(const TArrayView<const int16>& Wave)
{
	const int FadeSamples = (int)(FadeDurationSec * OutputSampleRate);
	const float InvFadeSamples = 1.0f / (float)FadeSamples;

	FadeBuffer.SetNumUninitialized(FadeSamples, EAllowShrinking::No);
	int16* Dst = FadeBuffer.GetData();
	const int16* Src = Wave.GetData() + (Wave.Num() - FadeSamples);

	for (int i = 0; i < FadeSamples; ++i)
	{
		Dst[i] = (int16)(Src[i] * (1.0f - (i * InvFadeSamples)));
	}

	QueueSamples(FadeBuffer.GetData(), FadeBuffer.Num());
}

void UHMIWavePlayerComponent::TickPlaybackProgress(float DeltaTime)
{
	if (!AudioComponent)
		return;

	PlaybackTS = GetAudioTS();
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f (%s)"), PlaybackTS, *UEnum::GetDisplayValueAsText(PlayState).ToString()));

	if (PrefetchTS > 0.0 && PlaybackTS >= PrefetchTS && AudioComponent->IsPlaying())
	{
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) Prefetch Drift=%f"), PlaybackTS, PlaybackTS - PrefetchTS);
		PrefetchTS = 0.0;
		PlayNextBuffer();
	}

	if (AutostopTS > 0.0 && PlaybackTS >= AutostopTS && AudioComponent->IsPlaying())
	{
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) Autostop Drift=%f"), PlaybackTS, PlaybackTS - AutostopTS);
		AutostopTS = 0.0;
		Stop(false);
	}

	// process finished waves before anything else
	for (auto& Op : PendingOperations)
	{
		if (Op.IsPlaying && PlaybackTS >= Op.FinishPlayTS)
		{
			Op.IsPlaying = false;

			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) WaveStop Id=%lld Duration=%f PlayDuration=%f Drift=%f"), 
				PlaybackTS, 
				Op.Input.OperationId,
				Op.Input.Wave.GetDuration(),
				PlaybackTS - Op.StartPlayTS,
				PlaybackTS - Op.FinishPlayTS
			);

			CompletedOperations.Add(Op.Input.OperationId);
			HandleOperationComplete(Op.Input, true);
		}
	}

	// remove finished waves
	for (int OperationId : CompletedOperations)
	{
		int Index = PendingOperations.IndexOfByPredicate([OperationId](const FHMIPlayWaveOperation& Iter) { return Iter.Input.OperationId == OperationId; });
		if (Index != INDEX_NONE)
			PendingOperations.RemoveAt(Index, EAllowShrinking::No);
	}

	// process playing waves
	if (AudioComponent->IsPlaying())
	{
		int NumPlayingWaves = 0;
		for (auto& Op : PendingOperations)
		{
			if (PlaybackTS >= Op.StartPlayTS && PlaybackTS < Op.FinishPlayTS)
			{
				bool WasStarted = false;

				if (!Op.IsPlaying)
				{
					Op.IsPlaying = true;
					WasStarted = true;

					CurrentWave = Op.Input.Wave;
					CurrentSequence = Op.Input.Sequence;

					UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) WaveStart Id=%lld Duration=%f Drift=%f"), 
						PlaybackTS, Op.Input.OperationId, Op.Input.Wave.GetDuration(), PlaybackTS - Op.StartPlayTS);

					OnWaveStart.Broadcast(Op.Input);
				}

				const float PlaybackSeconds = PlaybackTS - Op.StartPlayTS;
				OnWaveProgress.Broadcast(WasStarted, PlaybackSeconds, Op.Input.Wave.GetDuration());

				++NumPlayingWaves;
			}
		}

		if (NumPlayingWaves > 1)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("MULTIPLE PLAYING WAVES!"));
		}
	}

	// report underflow
	if (PlaybackTS >= UnderflowTS && LastReportedUnderflowTS < UnderflowTS)
	{
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) Underflow Drift=%f"), PlaybackTS, PlaybackTS - UnderflowTS);

		LastReportedUnderflowTS = UnderflowTS;

		OnPlayerUnderflow.Broadcast();
	}

	CompletedOperations.Reset();
}

void UHMIWavePlayerComponent::Stop(bool FadeOut)
{
	InputQueue.Reset();

	if (IsPlaying())
	{
		if (FadeOut && PlaybackTS < UnderflowTS) // buffer still playing
		{
			UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) StopFadeOut"), PlaybackTS);

			AudioComponent->FadeOut(FadeDurationSec, 0.0f); // trigger OnAudioPlayStateChanged -> ResetAudio
			AutostopTS = PlaybackTS + FadeDurationSec + 0.2f; // just in case, will be ignored if already stopped
		}
		else
		{
			UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) Stop"), PlaybackTS);

			AudioComponent->Stop(); // trigger OnAudioPlaybackFinished -> ResetAudio
		}
	}
}

void UHMIWavePlayerComponent::ResetAudio()
{
	if (ProceduralWave)
		ProceduralWave->ResetAudio();

	PendingOperations.Reset();
	InputQueue.Reset();

	CurrentWave.Reset();
	CurrentSequence.Reset();

	UnderflowTS = 0;
	PrefetchTS = 0;
	AutostopTS = 0;
	LastReportedUnderflowTS = 0;

	OnPlayerStop.Broadcast();
}

void UHMIWavePlayerComponent::OnAudioPlaybackFinished(UAudioComponent* Comp)
{
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) OnAudioPlaybackFinished"), PlaybackTS);

	ResetAudio();
}

void UHMIWavePlayerComponent::OnAudioPlayStateChanged(const UAudioComponent* Comp, EAudioComponentPlayState NewState)
{
	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) OnAudioPlayStateChanged: %s"), PlaybackTS, *UEnum::GetDisplayValueAsText(NewState).ToString());

	PlayState = NewState;
}

// dont use, bugged
void UHMIWavePlayerComponent::OnAudioPlaybackPercent(const UAudioComponent* Comp, const USoundWave* Wave, const float Percent)
{
}

bool UHMIWavePlayerComponent::IsPlaying() const
{
	//return AudioComponent && AudioComponent->IsPlaying();
	return AudioComponent && (AudioComponent->GetPlayState() == EAudioComponentPlayState::Playing);
}

double UHMIWavePlayerComponent::GetAudioTS() const
{
	return GetWorld()->GetAudioTimeSeconds();
}

void UHMIWavePlayerComponent::HandleOperationComplete(FHMIPlayWaveInput& Input, bool Success)
{
	FHMIPlayWaveResult Result(UHMIStatics::GetTimestamp(this), Success);

	if (Input.CompleteFunc)
	{
		Input.CompleteFunc(Input, Result);
	}
	else
	{
		OnWaveStop.Broadcast(Input, Result);
	}
}

#undef LOGPREFIX
