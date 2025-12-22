#pragma once

#include "HMIMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "HMIParameterRemapAsset.generated.h"

USTRUCT(BlueprintType, Category="HMI|Remap")
struct HMI_API FHMIParameterRemapInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	FName Name = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	FRuntimeFloatCurve Curve;
};

USTRUCT(BlueprintType, Category="HMI|Remap")
struct HMI_API FHMIParameterRemapOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(ClampMin="0.0", ClampMax="50.0", UIMin="0.0", UIMax="50.0"))
	float InterpSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	FRuntimeFloatCurve Curve;
};

USTRUCT(BlueprintType, Category="HMI|Remap")
struct HMI_API FHMIParameterRemapGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(TitleProperty="Name"))
	TArray<FHMIParameterRemapInput> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(TitleProperty="Name"))
	TArray<FHMIParameterRemapOutput> Outputs;
};

UCLASS(BlueprintType, ClassGroup="HMI|Remap")
class HMI_API UHMIParameterRemapAsset : public UDataAsset
{
	GENERATED_BODY()
	
	public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Clamp")
	float RawInputMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Clamp")
	float RawInputMax = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Clamp")
	float CombinedInputsMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Clamp")
	float CombinedInputsMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Clamp")
	float RawOutputMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI|Clamp")
	float RawOutputMax = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(ClampMin="0.0", ClampMax="50.0", UIMin="0.0", UIMax="50.0"))
	float InterpSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HMI", meta=(TitleProperty="Name"))
	TArray<FHMIParameterRemapGroup> Groups;
};
