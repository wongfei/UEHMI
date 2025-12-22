#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "HMIBuffer.h"
#include "HMINode_Resample.generated.h"

USTRUCT(BlueprintType, Category="HMI|Resample")
struct HMI_API FHMIResampleResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	FHMIWaveHandle Wave;

	UPROPERTY(BlueprintReadWrite, Category="HMI")
	int SampleRate = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMIOnResampleComplete, const FHMIResampleResult&, Result);

UCLASS()
class HMI_API UHMINode_Resample : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnResampleComplete Complete;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="Resample", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
	static UHMINode_Resample* Resample_Async(UObject* WorldContextObject, FHMIWaveHandle Wave, int OutputSampleRate = 0); // 0 -> HMI_DEFAULT_SAMPLE_RATE

	virtual void Activate() override;

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	FHMIWaveHandle Wave;

	UPROPERTY(Transient)
	int OutputSampleRate;
};
