#pragma once

#include "HMIMinimal.h"
#include "HMIProcessorResult.generated.h"

USTRUCT(BlueprintType, Category="HMI|Processor")
struct HMI_API FHMIProcessorResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	double Timestamp = 0;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	bool Success = false;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	FString Error;

	FHMIProcessorResult() {}

	FHMIProcessorResult(double InTimestamp, bool InSuccess) : 
		Timestamp(InTimestamp), Success(InSuccess) {}

	FHMIProcessorResult(double InTimestamp, bool InSuccess, FString InError) : 
		Timestamp(InTimestamp), Success(InSuccess), Error(MoveTemp(InError)) {}
};
