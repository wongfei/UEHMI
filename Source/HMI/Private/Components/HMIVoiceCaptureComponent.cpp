#include "Components/HMIVoiceCaptureComponent.h"

#include "VoiceModule.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#if PLATFORM_ANDROID
#include "AndroidPermissionCallbackProxy.h"
#include "AndroidPermissionFunctionLibrary.h"
#endif

DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("VoiceCapture_Process"), STAT_HMI_VoiceCapture_Process, STATGROUP_HMI)

#define LOGPREFIX "[VoiceCapture] "

UHMIVoiceCaptureComponent::UHMIVoiceCaptureComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UHMIVoiceCaptureComponent::UHMIVoiceCaptureComponent(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIVoiceCaptureComponent::~UHMIVoiceCaptureComponent()
{
}

void UHMIVoiceCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AutostartCapture)
	{
		StartCapture();
	}
}

void UHMIVoiceCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCapture();

	Super::EndPlay(EndPlayReason);
}

void UHMIVoiceCaptureComponent::WarmUp()
{
	if (VoiceCapture)
		return;

	bIsWarmingUp = true;
	StartCapture();
}

void UHMIVoiceCaptureComponent::StartCapture()
{
	if (VoiceCapture)
		return;

	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "StartCapture"));

	if (SampleRate == 0)
	{
		SampleRate = HMI_DEFAULT_SAMPLE_RATE;
	}

	if (!UHMIBufferStatics::IsValidSampleRate(SampleRate))
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Invalid sample rate: %d"), SampleRate);
		return;
	}

	#if PLATFORM_ANDROID
	FString AudioPermission = TEXT("android.permission.RECORD_AUDIO");
	if (!UAndroidPermissionFunctionLibrary::CheckPermission(AudioPermission))
	{
		TArray<FString> PermissionsToCheck;
		PermissionsToCheck.Add(AudioPermission);

		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "AcquirePermissions"));
		UAndroidPermissionCallbackProxy* PermCallback = UAndroidPermissionFunctionLibrary::AcquirePermissions(PermissionsToCheck);
		if (PermCallback)
		{
			PermCallback->OnPermissionsGrantedDelegate.BindUFunction(this, "PermissionCallback");
		}
	}
	else
	{
		StartCaptureDevice();
	}
	#else
	StartCaptureDevice();
	#endif
}

void UHMIVoiceCaptureComponent::StopCapture()
{
	if (VoiceCapture)
	{
		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "StopCapture"));
	}

	StopCaptureDevice();

	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "StopCapture DONE"));
}

void UHMIVoiceCaptureComponent::PermissionCallback(const TArray<FString>& Permissions, const TArray<bool>& GrantResults)
{
	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "PermissionCallback"));

	for (int PermId = 0; PermId < Permissions.Num(); PermId++)
	{
		if (!GrantResults[PermId])
		{
			UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Permission not granted: %s"), *Permissions[PermId]);
		}
	}

	StartCaptureDevice();
}

void UHMIVoiceCaptureComponent::StartCaptureDevice()
{
	if (VoiceCapture)
		return;

	OutputFormat = FHMIWaveFormat::Make_PCM_S16(SampleRate, HMI_VOICE_CHANNELS);

	VoiceBuffer.Reserve(4096);
	SentenceBuffer.Reserve(SampleRate * 5); // NumSamples = SampleRate(HZ) * Duration(s)

	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicInputGain"))->Set(InputGain);
	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.JitterBufferDelay"))->Set(JitterBufferDelay);

	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.SilenceDetectionThreshold"))->Set(SilenceThreshold);
	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.SilenceDetectionAttackTime"))->Set(SilenceAttackTime);
	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.SilenceDetectionReleaseTime"))->Set(SilenceReleaseTime);

	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicNoiseGateThreshold"))->Set(NoiseThreshold);
	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicNoiseAttackTime"))->Set(NoiseAttackTime);
	IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicNoiseReleaseTime"))->Set(NoiseReleaseTime);

	if (!FVoiceModule::Get().IsVoiceEnabled())
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Voice module not enabled: DefaultEngine.ini [Voice] bEnabled=True"));
		return;
	}

	auto VoiceCaptureTmp = FVoiceModule::Get().CreateVoiceCapture(DeviceName, SampleRate, HMI_VOICE_CHANNELS);
	if (!VoiceCaptureTmp)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "FVoiceModule::CreateVoiceCapture failed"));
		return;
	}

	if (!VoiceCaptureTmp->Start())
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "IVoiceCapture::Start failed"));
		return;
	}

	VoiceCapture = MoveTemp(VoiceCaptureTmp);
	LastVoiceActivity = GetWorld()->GetAudioTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(VoiceCaptureTimer, this, &UHMIVoiceCaptureComponent::PollDevice, PollRate, true);
}

void UHMIVoiceCaptureComponent::StopCaptureDevice()
{
	if (VoiceCapture)
	{
		GetWorld()->GetTimerManager().ClearTimer(VoiceCaptureTimer);
		VoiceCapture->Shutdown();
		VoiceCapture = nullptr;
	}

	AudioProcessor.Reset();
}

void UHMIVoiceCaptureComponent::PollDevice()
{
	if (!VoiceCapture)
		return;

	const uint64 CyclesStart = FPlatformTime::Cycles64();

	uint32 AvailableVoiceData = 0;
	uint32 VoiceDataCaptured = 0;

	for (;;)
	{
		AvailableVoiceData = 0;
		EVoiceCaptureState::Type CaptureState = VoiceCapture->GetCaptureState(AvailableVoiceData);

		// uninitialized
		if (CaptureState == EVoiceCaptureState::UnInitialized)
		{
			UE_LOG(LogHMI, Warning, TEXT(LOGPREFIX "CaptureState == UnInitialized"));

			if (!VoiceCapture->Init(DeviceName, SampleRate, HMI_VOICE_CHANNELS))
			{
				UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "IVoiceCapture::Init failed"));
				break;
			}

			if (!VoiceCapture->Start())
			{
				UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "IVoiceCapture::Start failed"));
				break;
			}

			UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "IVoiceCapture restarted"));
			break;
		}

		// no data
		if (CaptureState == EVoiceCaptureState::NoData || AvailableVoiceData == 0)
		{
			break;
		}

		// error
		if (CaptureState != EVoiceCaptureState::Ok)
		{
			UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Invalid EVoiceCaptureState: %s"), EVoiceCaptureState::ToString(CaptureState));
			break;
		}

		VoiceDataCaptured = 0;
		VoiceBuffer.SetNumUninitialized(AvailableVoiceData, EAllowShrinking::No);
		CaptureState = VoiceCapture->GetVoiceData(VoiceBuffer.GetData(), VoiceBuffer.Num(), VoiceDataCaptured); // PCM16

		// error
		if (CaptureState != EVoiceCaptureState::Ok || VoiceDataCaptured == 0)
		{
			UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "IVoiceCapture::GetVoiceData failed: %s"), EVoiceCaptureState::ToString(CaptureState));
			break;
		}

		if (!bIsWarmingUp)
		{
			VoiceBuffer.SetNum(VoiceDataCaptured, EAllowShrinking::No);
			HandleVoiceCaptured();
		}
	}

	if (!bIsWarmingUp)
	{
		DetectSentence();
	}

	const uint64 CyclesEnd = FPlatformTime::Cycles64();
	const float Elapsed = (float)FPlatformTime::ToMilliseconds64(CyclesEnd - CyclesStart);
	SET_FLOAT_STAT(STAT_HMI_VoiceCapture_Process, Elapsed)

	if (bIsWarmingUp)
	{
		bIsWarmingUp = false;
		StopCaptureDevice();
	}
}

void UHMIVoiceCaptureComponent::HandleVoiceCaptured()
{
	const auto NumSamples = VoiceBuffer.Num() / sizeof(int16);
	if (NumSamples <= 0)
		return;

	const double Timestamp = GetWorld()->GetAudioTimeSeconds();

	LastMicActivity = Timestamp;

	if (EnableAudioProcessor)
	{
		if (!AudioProcessor)
		{
			AudioProcessor = MakeUnique<FHMIAudioProcessor>(ProcessorConfig);
		}
		
		if (AudioProcessor->IsLevelControlEnabled())
		{
			const float Level = InputGain / InputGainRange;
			AudioProcessor->SetCurrentLevel(Level);
		}

		AudioProcessor->Process(VoiceBuffer, NumSamples, SampleRate);
		VoiceProb = AudioProcessor->GetVoiceProb();

		if (AudioProcessor->IsLevelControlEnabled())
		{
			static IConsoleVariable* MicInputGainCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicInputGain"));
			if (MicInputGainCVar)
			{
				const float Level = AudioProcessor->GetRecommendedLevel();
				InputGain = Level * InputGainRange;
				MicInputGainCVar->Set(InputGain);
			}
		}

		if (VoiceProb > MinVoiceProbability)
		{
			LastVoiceActivity = Timestamp;

			if (!bSentenceStarted)
			{
				bSentenceStarted = true;
				FirstVoiceActivity = Timestamp;
				UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) StartSentence"), GetWorld()->GetAudioTimeSeconds());
			}
		}
	}
	else // not using EnableAudioProcessor
	{
		LastVoiceActivity = Timestamp;

		if (!bSentenceStarted)
		{
			bSentenceStarted = true;
			FirstVoiceActivity = Timestamp;
			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) StartSentence"), GetWorld()->GetAudioTimeSeconds());
		}
	}

	if (VoiceBuffer.Num() > 0)
	{
		SentenceBuffer.Append(VoiceBuffer);
		OnVoiceCaptured.Broadcast(VoiceBuffer);
	}
}

void UHMIVoiceCaptureComponent::DetectSentence()
{
	const double Timestamp = GetWorld()->GetAudioTimeSeconds();

	const int SentenceSamples = SentenceBuffer.Num() / sizeof(int16);
	const float SentenceDuration = SentenceSamples / (float)SampleRate;

	const bool Cutoff = (
		Timestamp - LastMicActivity >= SentenceMicCutoff 
		|| (bSentenceStarted && Timestamp - LastVoiceActivity >= SentenceVoiceCutoff));

	if (bSentenceStarted && Cutoff && SentenceDuration >= SentenceMinimalDuration)
	{
		bSentenceStarted = false;
		HandleSentenceCaptured(SentenceDuration);
		SentenceBuffer.Reset();
		if (AudioProcessor) AudioProcessor->Flush();
	}
	else if (Cutoff && SentenceDuration > 0.0f)
	{
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "(%f) IgnoreNoise"), GetWorld()->GetAudioTimeSeconds());
		bSentenceStarted = false;
		SentenceBuffer.Reset();
		if (AudioProcessor) AudioProcessor->Flush();
	}
}

void UHMIVoiceCaptureComponent::HandleSentenceCaptured(float Duration)
{
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "(%f) SentenceCaptured"), GetWorld()->GetAudioTimeSeconds());
	UE_LOG(LogHMI, Log, TEXT(LOGPREFIX ">>> Duration=%.4f"), Duration);

	OnSentenceCaptured.Broadcast(FHMIWaveHandle(OutputFormat, SentenceBuffer));
}

double UHMIVoiceCaptureComponent::GetTimestamp() const
{
	return GetWorld()->GetAudioTimeSeconds();
}

float UHMIVoiceCaptureComponent::GetCurrentAmplitude() const
{
	return VoiceCapture ? VoiceCapture->GetCurrentAmplitude() : 0.0f;
}

float UHMIVoiceCaptureComponent::GetSentenceDuration() const
{
	const int SentenceSamples = SentenceBuffer.Num() / sizeof(int16);
	const float SentenceDuration = SentenceSamples / (float)SampleRate;
	return SentenceDuration;
}

#undef LOGPREFIX
