#pragma once

#include "HMIMinimal.h"
#include "HMIProcessorInput.generated.h"

USTRUCT(BlueprintType, Category="HMI|Processor")
struct HMI_API FHMIProcessorInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	int64 OperationId = HMI_NULL_OPERATION;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	double Timestamp = 0;

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FName UserTag = NAME_None;

	using TCompleteFunc = TFunction<void(FHMIProcessorInput&, struct FHMIProcessorResult&)>;
	TCompleteFunc CompleteFunc;
};
