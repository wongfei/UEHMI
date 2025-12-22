#include "OVR/HMILipSync_OVR.h"
#include "HMIProcessorImpl.h"
#include "HMISubsystemStatics.h"

#if HMI_WITH_OVRLIPSYNC
	#include "OVRLipSyncContext.h"
	#include "Audio/HMIResampler.h"
#endif

HMI_PROC_DECLARE_STATS(LipSync_OVR)

#define LOGPREFIX "[LipSync_OVR] "

const FName UHMILipSync_OVR::ImplName = TEXT("LipSync_OVR");

class UHMILipSync* UHMILipSync_OVR::GetOrCreateLipSync_OVR(UObject* WorldContextObject, 
	FName Name,
	FString ModelName,
	bool Accelerate,
	int SampleRate
)
{
	UHMILipSync* Proc = UHMISubsystemStatics::GetOrCreateLipSync(WorldContextObject, ImplName, Name);

	Proc->SetProcessorParam("ModelName", ModelName);
	Proc->SetProcessorParam("Accelerate", Accelerate);
	Proc->SetProcessorParam("SampleRate", SampleRate);

	return Proc;
}

UHMILipSync_OVR::UHMILipSync_OVR(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProviderName = ImplName;
	ProcessorName = ImplName.ToString();

	SetProcessorParam("ModelName", TEXT("ovrlipsync_offline_model.pb"));
	SetProcessorParam("Accelerate", false);
	SetProcessorParam("SampleRate", HMI_DEFAULT_SAMPLE_RATE);

	WeightMapping = NewObject<UHMIIndexMapping>();

	#if HMI_WITH_OVRLIPSYNC
		#define ADD_MAPPING(Name) WeightMapping->HMI_AddMapping((int32)ovrLipSyncViseme_##Name, #Name)
		ADD_MAPPING(sil);
		ADD_MAPPING(PP);
		ADD_MAPPING(FF);
		ADD_MAPPING(TH);
		ADD_MAPPING(DD);
		ADD_MAPPING(kk);
		ADD_MAPPING(CH);
		ADD_MAPPING(SS);
		ADD_MAPPING(nn);
		ADD_MAPPING(RR);
		ADD_MAPPING(aa);
		ADD_MAPPING(E);
		ADD_MAPPING(ih);
		ADD_MAPPING(oh);
		ADD_MAPPING(ou);
		#undef ADD_MAPPING
	#endif
}

UHMILipSync_OVR::UHMILipSync_OVR(FVTableHelper& Helper) : Super(Helper)
{
}

UHMILipSync_OVR::~UHMILipSync_OVR()
{
}

#if HMI_WITH_OVRLIPSYNC

bool UHMILipSync_OVR::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	FString ModelName = GetProcessorString("ModelName");
	HMI_GUARD_PARAM_NOT_EMPTY(ModelName);

	FString ModelPath = UHMIStatics::LocateDataFile(ModelName);
	if (!FPaths::FileExists(ModelPath))
	{
		ProcError(FString::Printf(TEXT("Model not found: %s"), *ModelPath));
		return false;
	}

	LipSyncSampleRate = GetProcessorInt("SampleRate");

	if (!LipSyncSampleRate) 
		LipSyncSampleRate = HMI_DEFAULT_SAMPLE_RATE;

	if (!UHMIBufferStatics::IsValidSampleRate(LipSyncSampleRate))
	{
		ProcError(FString::Printf(TEXT("Invalid sample rate: %d"), LipSyncSampleRate));
		return false;
	}

	const bool Accelerate = GetProcessorBool("Accelerate");

	auto TmpContext = MakeUnique<FOVRLipSyncContext>();
	if (!TmpContext->CreateContext(ovrLipSyncContextProvider_EnhancedWithLaughter, ModelPath, LipSyncSampleRate, Accelerate))
	{
		ProcError(TEXT("Failed to init OVRLipSyncContext"));
		return false;
	}

	Resampler = MakeUnique<FHMIResampler>();
	Resampler->InitDefaultVoice();

	Context = MoveTemp(TmpContext);
	return true;
}

void UHMILipSync_OVR::Proc_Release()
{
	Context.Reset();
	Resampler.Reset();

	Super::Proc_Release();
}

int64 UHMILipSync_OVR::ProcessInput(FHMILipSyncInput&& Input)
{
	const int64 OperationId = EnqueOperation(InputQueue, MoveTemp(Input), GetWorldContext());
	StartOrWakeProcessor();
	return OperationId;
}

void UHMILipSync_OVR::CancelOperation(bool PurgeQueue)
{
	Super::CancelOperation(PurgeQueue);

	if (PurgeQueue)
		PurgeInputQueue(InputQueue);
}

bool UHMILipSync_OVR::Proc_DoWork(int& QueueLength)
{
	FHMILipSync_OVR_Operation Operation;

	if (!DequeOperation(InputQueue, Operation, QueueLength))
		return false;

	bool Success = false;
	FString ErrorText;
	FHMILipSyncSequenceHandle Sequence;

	do
	{
		auto& Wave = Operation.Input.Wave;

		if (Wave.IsEmpty())
		{
			ErrorText = TEXT("Empty input");
			break;
		}

		if (!UHMIBufferStatics::IsValidVoice(Wave))
		{
			ErrorText = TEXT("Unsupported wave format");
			break;
		}

		HMI_PROC_PRE_WORK_STATS(LipSync_OVR)

		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "<<< Duration=%.4f"), Wave.GetDuration());

		TArrayView<const int16> SamplesView = Wave;

		if (Wave.GetSampleRate() != LipSyncSampleRate)
		{
			Resampler->Convert(SamplesView, Wave.GetSampleRate(), ResampledS16, LipSyncSampleRate);
			SamplesView = ResampledS16;
		}

		Sequence = FHMILipSyncSequenceHandle::Alloc();
		Success = ProcessSequence(SamplesView, Wave.GetNumChannels(), Sequence);
		if (!Success)
		{
			ErrorText = TEXT("ProcessSequence");
		}
	}
	while (false);

	if (!CancelFlag)
	{
		HandleOperationComplete(Success, MoveTemp(ErrorText), MoveTemp(Operation.Input), MoveTemp(Sequence));
	}

	return Success;
}

void UHMILipSync_OVR::Proc_PostWorkStats()
{
	HMI_PROC_POST_WORK_STATS(LipSync_OVR)
}

bool UHMILipSync_OVR::ProcessSequence(const TArrayView<const int16>& SamplesView, int NumChannels, FHMILipSyncSequenceHandle& Sequence)
{
	const int16* PCMData = SamplesView.GetData();
	const int32 NumSamples = SamplesView.Num();

	const float LipSyncFrameRate = 100.0f; // 100 hz, probably hardcoded to model
	const float LipSyncChunkDuration = 1.0f / LipSyncFrameRate;

	const auto DataType = (NumChannels == 2) ? ovrLipSyncAudioDataType_S16_Stereo : ovrLipSyncAudioDataType_S16_Mono;
	const int ChunkNFrames = (int)(LipSyncSampleRate * LipSyncChunkDuration);
	const int ChunkNSamples = ChunkNFrames * NumChannels;

	TArray<int16> Samples;
	TArray<float> Visemes;
	float LaughterScore = 0.0f;

	Samples.SetNumZeroed(ChunkNSamples, EAllowShrinking::No);

	int FrameDelay = 0; // milliseconds
	if (!Context->ProcessFrame(Samples.GetData(), ChunkNFrames, Visemes, LaughterScore, FrameDelay, DataType))
		return false;

	const int FrameOffset = (int)(LipSyncSampleRate * (FrameDelay * 0.001f) * NumChannels);

	for (int Offs = 0; Offs < NumSamples + FrameOffset; Offs += ChunkNSamples)
	{
		if (CancelFlag) 
			return false;

		const int RemainingSamples = NumSamples - Offs;
		if (RemainingSamples >= ChunkNSamples)
		{
			if (!Context->ProcessFrame(PCMData + Offs, ChunkNFrames, Visemes, LaughterScore, FrameDelay, DataType))
				return false;
		}
		else
		{
			if (RemainingSamples > 0)
			{
				FMemory::Memcpy(Samples.GetData(), PCMData + Offs, RemainingSamples * sizeof(int16));
				FMemory::Memset(Samples.GetData() + RemainingSamples, 0, (ChunkNSamples - RemainingSamples) * sizeof(int16));
			}
			else
			{
				FMemory::Memset(Samples.GetData(), 0, ChunkNSamples * sizeof(int16));
			}

			if (!Context->ProcessFrame(Samples.GetData(), ChunkNFrames, Visemes, LaughterScore, FrameDelay, DataType))
				return false;
		}

		if (Offs >= FrameOffset)
		{
			(*Sequence).Frames.Add({ WeightMapping, MoveTemp(Visemes), LaughterScore });
		}
	}

	(*Sequence).FrameRate = LipSyncFrameRate;

	return true;
}

void UHMILipSync_OVR::HandleOperationComplete(bool Success, FString&& Error, FHMILipSyncInput&& Input, FHMILipSyncSequenceHandle&& Sequence)
{
	if (Success && Sequence.IsValid())
	{
		UE_LOG(LogHMI, Log, TEXT(LOGPREFIX ">>> %d frames"), Sequence.GetFrames().Num());
	}

	FHMILipSyncResult Result(GetTimestamp(), Success, MoveTemp(Error), MoveTemp(Sequence));

	BroadcastResult(MoveTemp(Input), MoveTemp(Result), OnLipSyncComplete);
}

#endif // HMI_WITH_OVRLIPSYNC

#undef LOGPREFIX
