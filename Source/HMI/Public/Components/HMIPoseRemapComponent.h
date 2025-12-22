#pragma once

#include "HMIMinimal.h"
#include "Components/ActorComponent.h"
#include "HMIPoseRemapComponent.generated.h"

UCLASS(BlueprintType, ClassGroup="HMI|Remap", meta=(BlueprintSpawnableComponent))
class HMI_API UHMIPoseRemapComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FHMIPoseRemapAnimNode;

	public:

	UHMIPoseRemapComponent(const FObjectInitializer& ObjectInitializer);
	UHMIPoseRemapComponent(FVTableHelper& Helper);
	~UHMIPoseRemapComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	public:

	UFUNCTION(BlueprintCallable, Category="HMI|Remap")
	void SetInput(FName Name, float Value) { Inputs.Add(Name, Value); }

	UFUNCTION(BlueprintCallable, Category="HMI|Remap")
	void ResetInputs() { Inputs.Reset(); }

	UFUNCTION(BlueprintCallable, Category="HMI|Remap")
	void ZeroInputs();

	UFUNCTION(BlueprintCallable, Category="HMI|Remap")
	void SubmitUpdate();

	void Evaluate_AnyThread(class UHMIParameterRemapAsset* RemapAsset, struct FPoseContext& PoseContext, float DeltaTime);

	protected:

	TUniquePtr<class FHMIParameterRemapper> Remapper; // used by Evaluate_AnyThread!

	FCriticalSection InputsMux;
	TMap<FName, float> Inputs;
	TMap<FName, float> EvalInputs;

	std::atomic<bool> InputsModified;
};
