#pragma once

#include "HMIMinimal.h"
#include "OnnxProvider.generated.h"

UENUM(BlueprintType)
enum class EOnnxProvider : uint8
{
	Cpu = 0, 
	Cuda, 
	CoreML, 
	XNNPACK, 
	NNAPI, 
	TRT, 
	DirectML
};

inline FString GetSherpaProviderString(EOnnxProvider Provider)
{
	switch (Provider)
	{
		case EOnnxProvider::Cpu: return TEXT("cpu");
		case EOnnxProvider::Cuda: return TEXT("cuda");
		case EOnnxProvider::CoreML: return TEXT("coreml");
		case EOnnxProvider::XNNPACK: return TEXT("xnnpack");
		case EOnnxProvider::NNAPI: return TEXT("nnapi");
		case EOnnxProvider::TRT: return TEXT("trt");
		case EOnnxProvider::DirectML: return TEXT("directml");
	}
	
	return TEXT("cpu");
}
