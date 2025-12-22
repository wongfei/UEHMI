#pragma once

#include "HMIMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "HMIPoseRemapAnimNode.generated.h"

USTRUCT(BlueprintInternalUseOnly, Category="HMI|Remap")
struct HMI_API FHMIPoseRemapAnimNode : public FAnimNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Remap")
	FPoseLink Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Remap")
	TObjectPtr<class UHMIParameterRemapAsset> RemapAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Remap")
	TObjectPtr<class UHMIPoseRemapComponent> PoseRemapComponent;

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	// End of FAnimNode_Base interface
};
