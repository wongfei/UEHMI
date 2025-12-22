#pragma once

#include "HMIMinimal.h"
#include "HMIProcessorType.generated.h"

UENUM(BlueprintType)
enum class EHMIProcessorType : uint8
{
	Unknown = 0,
	TTS,
	STT,
	Chat,
	LipSync,
	FaceDetector,
};
