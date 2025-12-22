#include "ONNX/HMITextToSpeech_Piper.h"
#include "HMIProcessorImpl.h"
#include "HMISubsystemStatics.h"

#if HMI_WITH_PIPER
	#include <string>

	#include "HMIThirdPartyBegin.h"
	#include "piper_wrapper.h"
	#include "HMIThirdPartyEnd.h"
	
	#define PIPER_CALL(name) PiperApi->name##_fp
#endif

HMI_PROC_DECLARE_STATS(TTS_Piper)

#define LOGPREFIX "[TTS_Piper] "

const FName UHMITextToSpeech_Piper::ImplName = TEXT("TTS_Piper");

class UHMITextToSpeech* UHMITextToSpeech_Piper::GetOrCreateTTS_Piper(UObject* WorldContextObject, 
	FName Name,
	FString ModelName,
	bool Accelerate,
	FString VoiceId,
	float VoiceSpeed
)
{
	UHMITextToSpeech* Proc = UHMISubsystemStatics::GetOrCreateTTS(WorldContextObject, ImplName, Name);

	Proc->SetProcessorParam("ModelName", ModelName);
	Proc->SetProcessorParam("Accelerate", Accelerate);

	Proc->SetProcessorParam("VoiceId", VoiceId);
	Proc->SetProcessorParam("VoiceSpeed", VoiceSpeed);

	return Proc;
}

UHMITextToSpeech_Piper::UHMITextToSpeech_Piper(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProviderName = ImplName;
	ProcessorName = ImplName.ToString();

	SetProcessorParam("ModelName", TEXT("en_US-libritts_r-medium.onnx"));
	SetProcessorParam("Accelerate", false);

	SetProcessorParam("VoiceId", TEXT("0")); // string
	SetProcessorParam("VoiceSpeed", 1.0f); // larger -> faster
}

UHMITextToSpeech_Piper::UHMITextToSpeech_Piper(FVTableHelper& Helper) : Super(Helper)
{
}

UHMITextToSpeech_Piper::~UHMITextToSpeech_Piper()
{
}

#if HMI_WITH_PIPER

// piper uses espeak-ng which is epic fail by design and does not support multi-threading
#define HMI_PIPER_SINGLE_INSTANCE 1

#if HMI_PIPER_SINGLE_INSTANCE
static std::atomic<int> s_PiperInitialized;
#endif

void UHMITextToSpeech_Piper::StartOrWakeProcessor()
{
	Super::StartOrWakeProcessor();
}

bool UHMITextToSpeech_Piper::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	if (!PiperApi)
	{
		ProcError(TEXT("PiperApi is null"));
		return false;
	}

	ModelName = GetProcessorString("ModelName");
	HMI_GUARD_PARAM_NOT_EMPTY(ModelName);

	FString ModelPath = UHMIStatics::LocateDataFile(ModelName);
	if (!FPaths::FileExists(ModelPath))
	{
		ProcError(FString::Printf(TEXT("Model not found: %s"), *ModelName));
		return false;
	}

	FString SpeakDataPath = UHMIStatics::LocateDataSubdir(TEXT("espeak-ng-data"));
	if (!FPaths::DirectoryExists(SpeakDataPath))
	{
		ProcError(TEXT("espeak-ng-data not found"));
		return false;
	}
	
	const bool Accelerate = GetProcessorBool("Accelerate");

	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "ModelPath: %s"), *ModelPath);
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "SpeakDataPath: %s"), *SpeakDataPath);
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "Accelerate: %d"), Accelerate ? 1 : 0);

	#if HMI_PIPER_SINGLE_INSTANCE
	int Expected = 0;
	if (!s_PiperInitialized.compare_exchange_strong(Expected, 1))
	{
		ProcError(TEXT("Piper does not support multiple instances!"));
		return false;
	}
	#endif

	UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "piper_init"));
	PiperContext = PIPER_CALL(piper_init)(TCHAR_TO_UTF8(*ModelPath), TCHAR_TO_UTF8(*SpeakDataPath), Accelerate);
	if (!PiperContext)
	{
		const char* ErrorStr = PIPER_CALL(piper_get_error)();
		ProcError(FString::Printf(TEXT("piper_init: %s"), UTF8_TO_TCHAR(ErrorStr)));

		#if HMI_PIPER_SINGLE_INSTANCE
		s_PiperInitialized = 0;
		#endif

		return false;
	}

	ModelSampleRate = PIPER_CALL(piper_get_voice_sample_rate)(PiperContext);
	ModelSampleBits = PIPER_CALL(piper_get_voice_sample_bytes)(PiperContext) * 8;
	ModelNumChannels = PIPER_CALL(piper_get_voice_channels)(PiperContext);
	ModelNumSpeakers = PIPER_CALL(piper_get_num_speakers)(PiperContext);

	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "ModelSampleRate: %d"), ModelSampleRate);
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "ModelSampleBits: %d"), ModelSampleBits);
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "ModelNumChannels: %d"), ModelNumChannels);
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "ModelNumSpeakers: %d"), ModelNumSpeakers);

	if (!UHMIBufferStatics::IsValidSampleRate(ModelSampleRate))
	{
		ProcError(FString::Printf(TEXT("Not supported: ModelSampleRate=%d"), ModelSampleRate));
		return false;
	}

	if (ModelNumChannels != HMI_VOICE_CHANNELS)
	{
		ProcError(FString::Printf(TEXT("Not supported: ModelNumChannels=%d"), ModelNumChannels));
		return false;
	}

	if (ModelNumSpeakers <= 0)
	{
		ProcError(FString::Printf(TEXT("Invalid ModelNumSpeakers=%d"), ModelNumSpeakers));
		return false;
	}

	return true;
}

void UHMITextToSpeech_Piper::Proc_Release()
{
	if (PiperApi)
	{
		if (PiperContext)
		{
			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "piper_release"));
			PIPER_CALL(piper_release)(PiperContext);
			PiperContext = nullptr;

			#if HMI_PIPER_SINGLE_INSTANCE
			int Expected = 1;
			s_PiperInitialized.compare_exchange_strong(Expected, 0);
			#endif
		}
	}

	Super::Proc_Release();
}

int64 UHMITextToSpeech_Piper::ProcessInput(FHMITextToSpeechInput&& Input)
{
	const int64 OperationId = EnqueOperation(InputQueue, MoveTemp(Input), GetWorldContext());
	StartOrWakeProcessor();
	return OperationId;
}

void UHMITextToSpeech_Piper::CancelOperation(bool PurgeQueue)
{
	Super::CancelOperation(PurgeQueue);

	if (PurgeQueue)
		PurgeInputQueue(InputQueue);

	PiperCancelFlag = true;
}

bool UHMITextToSpeech_Piper::Proc_DoWork(int& QueueLength)
{
	FHMITextToSpeech_Piper_Operation Operation;

	if (!DequeOperation(InputQueue, Operation, QueueLength))
		return false;

	bool Success = false;
	FString ErrorText;
	FHMIWaveHandle OutputWave;
	piper_buffer_ptr PiperBuffer = nullptr;

	do
	{
		PiperCancelFlag = false;

		std::string InputTextUtf8(TCHAR_TO_UTF8(*Operation.Input.Text));
		if (InputTextUtf8.empty())
		{
			ErrorText = TEXT("Empty input");
			break;
		}

		HMI_PROC_PRE_WORK_STATS(TTS_Piper)

		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "<<< \"%s\""), *Operation.Input.Text);

		PiperBuffer = PIPER_CALL(piper_alloc_buffer)();
		if (!PiperBuffer)
		{
			ErrorText = TEXT("piper_alloc_buffer");
			break;
		}

		int VoiceId = 0;
		float VoiceSpeed = 1.0f;

		if (!Operation.Input.VoiceId.IsEmpty())
			VoiceId = FCString::Atoi(*Operation.Input.VoiceId);
		else
			VoiceId = GetProcessorInt("VoiceId");

		if (Operation.Input.Speed > 0.0f)
			VoiceSpeed = Operation.Input.Speed;
		else
			VoiceSpeed = GetProcessorFloat("VoiceSpeed");

		VoiceId = FMath::Clamp(VoiceId, 0, ModelNumSpeakers - 1);
		VoiceSpeed = FMath::Clamp(VoiceSpeed, 0.01f, 10.0f);
		const float LengthScale = 1.0f / VoiceSpeed;

		const int ErrorCode = PIPER_CALL(piper_text_to_buffer)(PiperContext, InputTextUtf8.c_str(), PiperBuffer, VoiceId, LengthScale, &PiperCancelFlag);
		HMI_BREAK_ON_CANCEL();

		if (ErrorCode != 0)
		{
			const char* ErrorStr = PIPER_CALL(piper_get_error)();
			ErrorText = FString::Printf(TEXT("piper_text_to_buffer: %s"), UTF8_TO_TCHAR(ErrorStr));
			break;
		}

		const void* RawData = PIPER_CALL(piper_get_buffer_data)(PiperBuffer);
		const size_t RawSize = PIPER_CALL(piper_get_buffer_size)(PiperBuffer);
		if (!RawData || !RawSize)
		{
			ErrorText = TEXT("piper_get_buffer_data");
			break;
		}

		OutputWave = GenerateOutputWave({(const int16*)RawData, (int32)(RawSize / sizeof(int16))}, ModelSampleRate, ModelNumChannels);
		Success = true;
	}
	while (false);

	if (PiperBuffer)
	{
		PIPER_CALL(piper_free_buffer)(PiperBuffer);
	}

	if (!CancelFlag)
	{
		HandleOperationComplete(Success, MoveTemp(ErrorText), MoveTemp(Operation.Input), MoveTemp(OutputWave));
	}

	return Success;
}

void UHMITextToSpeech_Piper::Proc_PostWorkStats()
{
	HMI_PROC_POST_WORK_STATS(TTS_Piper)
}

void UHMITextToSpeech_Piper::HandleOperationComplete(bool Success, FString&& Error, FHMITextToSpeechInput&& Input, FHMIWaveHandle&& Wave)
{
	if (Success)
	{
		UE_LOG(LogHMI, Log, TEXT(LOGPREFIX ">>> Duration=%.4f"), Wave.GetDuration());
	}

	Wave.SetTag(Input.UserTag);

	FHMITextToSpeechResult Result(GetTimestamp(), Success, MoveTemp(Error), MoveTemp(Wave));

	BroadcastResult(MoveTemp(Input), MoveTemp(Result), OnTextToSpeechComplete);
}

#endif // HMI_WITH_PIPER

#undef LOGPREFIX
