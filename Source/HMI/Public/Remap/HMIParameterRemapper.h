#pragma once

#include "HMIParameterRemapAsset.h"

class HMI_API FHMIParameterRemapper
{
	public:

	FHMIParameterRemapper();
	~FHMIParameterRemapper();

	void Update(class UHMIParameterRemapAsset* RemapAsset, float DeltaTime, float InterpSpeed = 0.0f, float InterpTolerance = 0.001f);

	void SetInput(const FName& Name, float Value) { Inputs.Add(Name, Value); }

	TMap<FName, float>& GetInputsMut() { return Inputs; }
	TMap<FName, float>& GetOutputsMut() { return Outputs; }

	const TMap<FName, float>& GetInputs() const { return Inputs; }
	const TMap<FName, float>& GetOutputs() const { return Outputs; }

	protected:

	void InterpolateParam(const FName& Key, float TargetValue, float DeltaTime, float InterpSpeed, float InterpTolerance);

	TMap<FName, float> Inputs;
	TMap<FName, float> Outputs;
};
