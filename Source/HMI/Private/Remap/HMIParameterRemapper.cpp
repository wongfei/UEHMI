#include "Remap/HMIParameterRemapper.h"

DECLARE_CYCLE_STAT(TEXT("RemapParameters"), STAT_HMI_RemapParameters, STATGROUP_HMI)

FHMIParameterRemapper::FHMIParameterRemapper()
{
}

FHMIParameterRemapper::~FHMIParameterRemapper()
{
}

void FHMIParameterRemapper::Update(class UHMIParameterRemapAsset* RemapAsset, float DeltaTime, float InterpSpeed, float InterpTolerance)
{
	//SCOPE_CYCLE_COUNTER(STAT_HMI_RemapParameters)

	if (RemapAsset)
	{
		const float RawInputMin = RemapAsset->RawInputMin;
		const float RawInputMax = RemapAsset->RawInputMax;

		const float CombinedInputsMin = RemapAsset->CombinedInputsMin;
		const float CombinedInputsMax = RemapAsset->CombinedInputsMax;

		const float RawOutputMin = RemapAsset->RawOutputMin;
		const float RawOutputMax = RemapAsset->RawOutputMax;

		const float DefInterpSpeed = RemapAsset->InterpSpeed;

		for (const FHMIParameterRemapGroup& Group : RemapAsset->Groups)
		{
			float CombinedInputsValue = 0.0f;

			for (const FHMIParameterRemapInput& Input : Group.Inputs)
			{
				const float* InputValuePtr = Inputs.Find(Input.Name);
				if (InputValuePtr)
				{
					float InputValue = *InputValuePtr;

					if (RawInputMin < RawInputMax)
					{
						InputValue = FMath::Clamp(InputValue, RawInputMin, RawInputMax);
					}

					const FRichCurve* Curve = Input.Curve.GetRichCurveConst();
					if (Curve && Curve->GetNumKeys() >= 2)
					{
						InputValue = Curve->Eval(InputValue) * Input.Weight;
					}
					else
					{
						InputValue = InputValue * Input.Weight;
					}

					CombinedInputsValue += InputValue;
				}
			}

			if (CombinedInputsMin < CombinedInputsMax)
			{
				CombinedInputsValue = FMath::Clamp(CombinedInputsValue, CombinedInputsMin, CombinedInputsMax);
			}

			for (const FHMIParameterRemapOutput& Output : Group.Outputs)
			{
				float OutputValue = CombinedInputsValue;

				const FRichCurve* Curve = Output.Curve.GetRichCurveConst();
				if (Curve && Curve->GetNumKeys() >= 2)
				{
					OutputValue = Curve->Eval(OutputValue) * Output.Weight;
				}
				else
				{
					OutputValue = OutputValue * Output.Weight;
				}

				if (RawOutputMin < RawOutputMax)
				{
					OutputValue = FMath::Clamp(OutputValue, RawOutputMin, RawOutputMax);
				}

				float Speed = InterpSpeed;
				if (Speed <= 0.0f)
				{
					Speed = Output.InterpSpeed;
					if (Speed <= 0.0f)
					{
						Speed = DefInterpSpeed;
					}
				}

				InterpolateParam(Output.Name, OutputValue, DeltaTime, Speed, InterpTolerance);
			}
		}
	}
	else // one to one
	{
		for (const auto& Input : Inputs)
		{
			InterpolateParam(Input.Key, Input.Value, DeltaTime, InterpSpeed, InterpTolerance);
		}
	}
}

void FHMIParameterRemapper::InterpolateParam(const FName& Key, float TargetValue, float DeltaTime, float InterpSpeed, float InterpTolerance)
{
	float& OutValue = Outputs.FindOrAdd(Key);

	if (!FMath::IsNearlyEqual(OutValue, TargetValue, InterpTolerance))
	{
		if (InterpSpeed > 0.0f)
		{
			OutValue = FMath::FInterpTo(OutValue, TargetValue, DeltaTime, InterpSpeed);
		}
		else
		{
			OutValue = TargetValue;
		}
	}
}
