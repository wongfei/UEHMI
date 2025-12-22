#pragma once

#include "Remap/HMIIndexMapping.h"
#include "HMILipSyncSequence.generated.h"

USTRUCT(BlueprintType, Category="HMI|LipSync")
struct HMI_API FHMILipSyncFrame
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	TObjectPtr<UHMIIndexMapping> WeightMapping;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	TArray<float> Weights;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	float LaughterScore = 0;

	float GetWeight(int Id) const { return HMI_GetValue(Weights, Id); }
	float GetWeight(const FName& Name) const { return HMI_GetValue(WeightMapping, Weights, Name); }
	FName GetWeightName(int Id) const { return HMI_GetName(WeightMapping, Id); }
};

USTRUCT(BlueprintType, Category="HMI|LipSync")
struct HMI_API FHMILipSyncSequence
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	float FrameRate = 0;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	TArray<FHMILipSyncFrame> Frames;
};

USTRUCT(BlueprintType, Category="HMI|LipSync")
struct HMI_API FHMILipSyncSequenceHandle
{
	GENERATED_BODY()

	mutable TSharedPtr<FHMILipSyncSequence> Ptr;

	static FHMILipSyncSequenceHandle Alloc() { return FHMILipSyncSequenceHandle{ MakeShared<FHMILipSyncSequence>() }; }
	void Reset() { Ptr.Reset(); }
	const bool IsValid() const { return Ptr.IsValid(); }
	void Check() const { check(IsValid()); }

	FHMILipSyncSequence& operator*() { Check(); return *Ptr; }
	const FHMILipSyncSequence& operator*() const { Check(); return *Ptr; }

	const TArray<FHMILipSyncFrame>& GetFrames() const { Check(); return Ptr->Frames; }
	TArray<FHMILipSyncFrame>& GetFrames() { Check(); return Ptr->Frames; }

	float GetFrameRate() const { Check(); return Ptr->FrameRate; }
};
