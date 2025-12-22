#pragma once

#include "HMIMinimal.h"
#include "HMIProcessorState.generated.h"

UENUM(BlueprintType)
enum class EHMIProcessorState : uint8
{
	Stopped = 0,
	Starting,
	Busy,
	Idle,
	Stopping,
};
