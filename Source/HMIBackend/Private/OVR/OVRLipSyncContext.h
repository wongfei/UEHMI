#pragma once

#if HMI_WITH_OVRLIPSYNC

#include "HMIMinimal.h"

#include "HMIThirdPartyBegin.h"
#include "OVRLipSync.h"
#include "HMIThirdPartyEnd.h"

class FOVRLipSyncContext
{
	public:

	FOVRLipSyncContext();
	~FOVRLipSyncContext();

	static bool Init(int SampleRate, int BufferSize, FString DllPath);
	static void Shutdown();

	bool CreateContext(ovrLipSyncContextProvider Provider, FString ModelPath, int SampleRate, bool Accelerate);
	void DestroyContext();

	bool ProcessFrame(const int16* Samples, int NumSamples, TArray<float>& Visemes, float& LaughterScore, int& FrameDelay, ovrLipSyncAudioDataType DataType);
	void ProcessFrameAsync(const int16* Samples, int NumSamples, ovrLipSyncAudioDataType DataType);

	using AsyncCallbackType = TFunction<void(const TArrayView<const float>& Visemes, float LaughterScore)>;
	void SetAsyncCallback(AsyncCallbackType InCallback) { AsyncCallback = InCallback; }

	protected:

	static void OVRFrameCallback(void* ParamPtr, const ovrLipSyncFrame* Frame, ovrLipSyncResult ErrorCode);

	ovrLipSyncContext OVRContext = {};
	AsyncCallbackType AsyncCallback;
};

#endif
