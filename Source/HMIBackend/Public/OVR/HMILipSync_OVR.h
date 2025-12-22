#pragma once 

#include "Processors/HMILipSync.h"
#include "HMILipSync_OVR.generated.h"

USTRUCT()
struct HMIBACKEND_API FHMILipSync_OVR_Operation
{
	GENERATED_BODY()

	UPROPERTY()
	FHMILipSyncInput Input;
};

UCLASS()
class HMIBACKEND_API UHMILipSync_OVR : 
	public UHMILipSync,
	public THMIProcessorQueue<FHMILipSync_OVR_Operation, FHMILipSyncInput>
{
	GENERATED_BODY()

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMILipSync* GetOrCreateLipSync_OVR(UObject* WorldContextObject, 
		FName Name = NAME_None,
		FString ModelName = TEXT(""),
		bool Accelerate = false,
		int SampleRate = 0 // 0 -> HMI_DEFAULT_SAMPLE_RATE
	);

	UHMILipSync_OVR(const FObjectInitializer& ObjectInitializer);
	UHMILipSync_OVR(FVTableHelper& Helper);
	~UHMILipSync_OVR();

	#if HMI_WITH_OVRLIPSYNC

	public:

	virtual bool IsProcessorImplemented() const override { return true; }
	virtual int64 ProcessInput(FHMILipSyncInput&& Input) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;

	protected:

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	virtual bool ProcessSequence(const TArrayView<const int16>& SamplesView, int NumChannels, FHMILipSyncSequenceHandle& Sequence);
	virtual void HandleOperationComplete(bool Success, FString&& Error, FHMILipSyncInput&& Input, FHMILipSyncSequenceHandle&& Sequence);

	int LipSyncSampleRate = 0;
	TUniquePtr<class FOVRLipSyncContext> Context;
	TUniquePtr<class FHMIResampler> Resampler;
	TArray<int16> ResampledS16;

	#endif // HMI_WITH_OVRLIPSYNC

	protected:

	UPROPERTY(Transient)
	TObjectPtr<UHMIIndexMapping> WeightMapping;

	UPROPERTY(Transient)
	TArray<FHMILipSync_OVR_Operation> InputQueue;
};
