#include "Remap/HMIPoseRemapAnimNode.h"
#include "Components/HMIPoseRemapComponent.h"

#include "Animation/AnimTrace.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "GameFramework/Actor.h"

void FHMIPoseRemapAnimNode::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	Source.Initialize(Context);
}

void FHMIPoseRemapAnimNode::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	Source.CacheBones(Context);
}

// perform game-thread work prior to non-game thread Update() being called
void FHMIPoseRemapAnimNode::PreUpdate(const UAnimInstance* InAnimInstance)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(PreUpdate)

	if (!PoseRemapComponent)
	{
		AActor* Actor = InAnimInstance->GetOwningActor();
		if (Actor)
		{
			PoseRemapComponent = Actor->FindComponentByClass<UHMIPoseRemapComponent>();
		}
	}
}

void FHMIPoseRemapAnimNode::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread)
	GetEvaluateGraphExposedInputs().Execute(Context);
	Source.Update(Context);
}

inline void SetCurveValue(struct FPoseContext& Output, const FName& Name, float Value)
{
	Output.Curve.Set(Name, Value);
	TRACE_ANIM_NODE_VALUE(Output, *Name.ToString(), Value);
}

void FHMIPoseRemapAnimNode::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	Source.Evaluate(Output);

	if (PoseRemapComponent)
	{
		PoseRemapComponent->Evaluate_AnyThread(RemapAsset, Output, Output.AnimInstanceProxy->GetDeltaSeconds());
	}
}

void FHMIPoseRemapAnimNode::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	Source.GatherDebugData(DebugData);
}
