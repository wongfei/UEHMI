#include "OVR/OVRLipSyncContext.h"

#if HMI_WITH_OVRLIPSYNC

#define OVR_CALL(name, args) name args

#define LOGPREFIX "[OVR_LipSync] "

FOVRLipSyncContext::FOVRLipSyncContext()
{
}

FOVRLipSyncContext::~FOVRLipSyncContext()
{
	DestroyContext();
}

bool FOVRLipSyncContext::Init(int SampleRate, int BufferSize, FString DllPath) // static
{
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "Init"));

	ovrLipSyncResult Result = OVR_CALL(ovrLipSync_InitializeEx, (SampleRate, BufferSize, TCHAR_TO_UTF8(*DllPath)));

	if (Result != ovrLipSyncSuccess)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "ovrLipSync_InitializeEx: %d"), (int)Result);
		return false;
	}

	return true;
}

void FOVRLipSyncContext::Shutdown() // static
{
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "Shutdown"));
	OVR_CALL(ovrLipSync_Shutdown, ());
}

bool FOVRLipSyncContext::CreateContext(ovrLipSyncContextProvider Provider, FString ModelPath, int SampleRate, bool Accelerate)
{
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "CreateContext"));

	if (OVRContext)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "Already initialized"));
		return false;
	}

	ovrLipSyncResult Result = OVR_CALL(ovrLipSync_CreateContextWithModelFile, (&OVRContext, Provider, TCHAR_TO_UTF8(*ModelPath), SampleRate, Accelerate));

	if (Result != ovrLipSyncSuccess)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "ovrLipSync_CreateContextWithModelFile: %d"), (int)Result);
		return false;
	}

	return true;
}

void FOVRLipSyncContext::DestroyContext()
{
	if (OVRContext)
	{
		UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "DestroyContext"));
		OVR_CALL(ovrLipSync_DestroyContext, (OVRContext));
		OVRContext = 0;
	}
}

bool FOVRLipSyncContext::ProcessFrame(const int16* Samples, int NumSamples, TArray<float>& Visemes, float& LaughterScore, int& FrameDelay, ovrLipSyncAudioDataType DataType)
{
	if (Visemes.Num() != (int)ovrLipSyncViseme_Count)
	{
		Visemes.SetNumZeroed((int)ovrLipSyncViseme_Count, EAllowShrinking::No);
	}

	ovrLipSyncFrame Frame = {};
	Frame.visemes = Visemes.GetData();
	Frame.visemesLength = Visemes.Num();

	const auto ErrorCode = OVR_CALL(ovrLipSync_ProcessFrameEx, (OVRContext, Samples, NumSamples, DataType, &Frame));
	if (ErrorCode != ovrLipSyncSuccess)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "ovrLipSync_ProcessFrameEx: %d"), (int)ErrorCode);
		return false;
	}

	LaughterScore = Frame.laughterScore;
	FrameDelay = Frame.frameDelay;

	return true;
}

void FOVRLipSyncContext::ProcessFrameAsync(const int16* Samples, int NumSamples, ovrLipSyncAudioDataType DataType)
{
	const auto ErrorCode = OVR_CALL(ovrLipSync_ProcessFrameAsync, (OVRContext, Samples, NumSamples, DataType, OVRFrameCallback, this));
	if (ErrorCode != ovrLipSyncSuccess)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "ovrLipSync_ProcessFrameAsync: %d"), (int)ErrorCode);
		return;
	}
}

// called by OVR thread!
void FOVRLipSyncContext::OVRFrameCallback(void* ParamPtr, const ovrLipSyncFrame* Frame, ovrLipSyncResult ErrorCode)
{
	if (ErrorCode != ovrLipSyncSuccess)
	{
		UE_LOG(LogHMI, Error, TEXT(LOGPREFIX "OVRFrameCallback: %d"), (int)ErrorCode);
		return;
	}

	auto Impl = (FOVRLipSyncContext*)ParamPtr;
	if (Impl->AsyncCallback)
	{
		Impl->AsyncCallback({Frame->visemes, (int32)Frame->visemesLength}, Frame->laughterScore);
	}
}

#undef LOGPREFIX

#endif
