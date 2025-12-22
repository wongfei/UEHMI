#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Stats/Stats.h"

HMI_API DECLARE_LOG_CATEGORY_EXTERN(LogHMI, Log, All);

DECLARE_STATS_GROUP(TEXT("HMI"), STATGROUP_HMI, STATCAT_Advanced);

#define HMI_NULL_OPERATION 0
#define HMI_VOICE_CHANNELS 1
#define HMI_DEFAULT_SAMPLE_RATE 16000
