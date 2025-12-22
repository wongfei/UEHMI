#include "Components/HMIPoseRemapComponent.h"
#include "Remap/HMIParameterRemapper.h"

#include "Animation/AnimNodeBase.h"

UHMIPoseRemapComponent::UHMIPoseRemapComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UHMIPoseRemapComponent::UHMIPoseRemapComponent(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIPoseRemapComponent::~UHMIPoseRemapComponent()
{
}

void UHMIPoseRemapComponent::BeginPlay()
{
	Remapper = MakeUnique<FHMIParameterRemapper>();

	Super::BeginPlay();
}

void UHMIPoseRemapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Remapper.Reset();

	Super::EndPlay(EndPlayReason);
}

void UHMIPoseRemapComponent::ZeroInputs()
{
	for (auto& Input : Inputs)
	{
		Input.Value = 0.0f;
	}
}

void UHMIPoseRemapComponent::SubmitUpdate()
{
	{
		FScopeLock LOCK(&InputsMux);
		EvalInputs = Inputs;
	}
	InputsModified = true;
}

void UHMIPoseRemapComponent::Evaluate_AnyThread(class UHMIParameterRemapAsset* RemapAsset, struct FPoseContext& PoseContext, float DeltaTime)
{
	if (!RemapAsset || !Remapper)
		return;

	bool Expected = true;
	if (InputsModified.compare_exchange_weak(Expected, false))
	{
		FScopeLock LOCK(&InputsMux);
		Remapper->GetInputsMut() = EvalInputs;
	}

	Remapper->Update(RemapAsset, DeltaTime);

	const auto& Outputs = Remapper->GetOutputs();
	for (const auto& Output : Outputs)
	{
		PoseContext.Curve.Set(Output.Key, Output.Value);
	}
}
