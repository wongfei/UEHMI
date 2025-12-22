#include "HMIBackendModule.h"
#include "HMIStatics.h"

#include "Misc/Paths.h"

#if HMI_WITH_CUSTOM_OPENCV
	#include "OpenCV/OpenCVAllocator.h"
	#include "OpenCV/OpenCVIncludesBegin.h"
	#include "opencv2/unreal.hpp"
	#include "OpenCV/OpenCVIncludesEnd.h"
#endif

#if ORT_API_MANUAL_INIT
	#include "HMIThirdPartyBegin.h"
	#include "onnxruntime_cxx_api.h"
	#include "HMIThirdPartyEnd.h"
	typedef const OrtApiBase* (ORT_API_CALL *OrtGetApiBase_proto)(void) NO_EXCEPTION;
#endif

#if HMI_WITH_SHERPA
	#include "HMIThirdPartyBegin.h"
	#include "ONNX/SherpaLoader.h"
	#include "HMIThirdPartyEnd.h"
#endif

#if HMI_WITH_PIPER
	#include "HMIThirdPartyBegin.h"
	#include "piper_wrapper.h"
	#include "HMIThirdPartyEnd.h"
#endif

#if HMI_WITH_OVRLIPSYNC
	#include "OVR/OVRLipSyncContext.h"
#endif

void FHMIBackendModule::StartupModule()
{
	UE_LOG(LogHMI, Verbose, TEXT("FHMIBackendModule::StartupModule"));

	FPlatformProcess::PushDllDirectory(*UHMIStatics::GetPluginThirdPartyBinDir());

	#if HMI_WITH_CUSTOM_OPENCV
	// SEE -> Engine\Plugins\Runtime\OpenCV\Source\OpenCVHelper\Private\OpenCVHelperModule.cpp
	OpenCVHandle = FPlatformProcess::GetDllHandle(*UHMIStatics::GetPluginDllFilepath(TEXT("opencv_worldhmi")));
	if (!OpenCVHandle)
	{
		UE_LOG(LogHMI, Error, TEXT("Failed to load opencv_worldhmi"));
	}
	else
	{
		cv::unreal::SetMallocAndFree(&OpenCVAllocator::UnrealMalloc, &OpenCVAllocator::UnrealFree);
		bHaveOpenCV = true;
	}
	#endif

	#if HMI_WITH_ANY_ONNX
	// SEE -> Engine\Plugins\NNE\NNERuntimeORT
	OnnxHandle = FPlatformProcess::GetDllHandle(*UHMIStatics::GetPluginDllFilepath(TEXT("onnxruntime")));
	if (!OnnxHandle)
	{
		UE_LOG(LogHMI, Error, TEXT("Failed to load onnxruntime"));
	}
	else
	{
		FPlatformProcess::GetDllHandle(*UHMIStatics::GetPluginDllFilepath(TEXT("DirectML")));
		FPlatformProcess::GetDllHandle(*UHMIStatics::GetPluginDllFilepath(TEXT("DirectML.Debug")));
		bHaveOnnx = true;
	}
	#endif

	#ifdef ORT_API_MANUAL_INIT
	if (OnnxHandle)
	{
		bHaveOnnx = false;
		auto OrtGetApiBase_fp = (OrtGetApiBase_proto)FPlatformProcess::GetDllExport(OnnxHandle, TEXT("OrtGetApiBase"));
		if (ensure(OrtGetApiBase_fp))
		{
			auto ApiBase = OrtGetApiBase_fp();
			if (ensure(ApiBase))
			{
				auto Api = ApiBase->GetApi(ORT_API_VERSION);
				if (ensure(Api))
				{
					Ort::InitApi(Api);
					bHaveOnnx = true;
				}
			}
		}
	}
	#endif

	#if HMI_WITH_SHERPA
	SherpaAPI = MakeUnique<FSherpaAPI>();
	bHaveSherpa = LoadSherpaAPI(UHMIStatics::GetPluginDllFilepath(TEXT("sherpa-onnx-c-api")), *SherpaAPI);
	if (!bHaveSherpa)
	{
		UE_LOG(LogHMI, Error, TEXT("Failed to load sherpa-onnx-c-api"));
		SherpaAPI.Reset();
	}
	#endif

	#if HMI_WITH_PIPER
	const FString PiperDllPath = UHMIStatics::GetPluginDllFilepath(TEXT("piper"));
	PiperHandle = FPlatformProcess::GetDllHandle(*PiperDllPath);
	if (!PiperHandle)
	{
		UE_LOG(LogHMI, Error, TEXT("Failed to load piper"));
	}
	else
	{
		auto piper_get_api_fp = (piper_get_api_ft)FPlatformProcess::GetDllExport(PiperHandle, TEXT("piper_get_api"));
		if (ensure(piper_get_api_fp))
		{
			PiperAPI = MakeUnique<piper_api>();
			FMemory::Memset(PiperAPI.Get(), 0, sizeof(piper_api));

			if (ensure(piper_get_api_fp(PiperAPI.Get()) == 0))
			{
				if (ensure(PiperAPI->piper_init_fp))
				{
					bHavePiper = true;
				}
			}
		}
	}
	if (!bHavePiper && PiperAPI)
	{
		PiperAPI.Reset();
	}
	#endif
	
	#if HMI_WITH_OVRLIPSYNC
	const FString OVRLipSyncDir = FPaths::GetPath(UHMIStatics::GetPluginDllFilepath(TEXT("OVRLipSync")));
	bHaveOVRLipSync = FOVRLipSyncContext::Init(HMI_DEFAULT_SAMPLE_RATE, 4096, OVRLipSyncDir);
	if (!bHaveOVRLipSync)
	{
		UE_LOG(LogHMI, Error, TEXT("Failed to init OVRLipSync"));
	}
	#endif
}

void FHMIBackendModule::ShutdownModule()
{
	UE_LOG(LogHMI, Verbose, TEXT("FHMIBackendModule::ShutdownModule"));

	#if HMI_WITH_OVRLIPSYNC
	if (bHaveOVRLipSync)
	{
		FOVRLipSyncContext::Shutdown();
		bHaveOVRLipSync = false;
	}
	#endif

	#if HMI_WITH_PIPER
	if (bHavePiper)
	{
		PiperAPI.Reset();
		bHavePiper = false;
	}
	#endif

	#if HMI_WITH_SHERPA
	if (bHaveSherpa)
	{
		SherpaAPI.Reset();
		bHaveSherpa = false;
	}
	#endif

	#if HMI_WITH_CUSTOM_OPENCV
	if (bHaveOpenCV)
	{
		// better don't touch it!
		//cv::unreal::SetMallocAndFree(nullptr, nullptr);
		//FPlatformProcess::FreeDllHandle(OpenCVHandle);
		OpenCVHandle = nullptr;
		bHaveOpenCV = false;
	}
	#endif
}

IMPLEMENT_MODULE(FHMIBackendModule, HMIBackend)
