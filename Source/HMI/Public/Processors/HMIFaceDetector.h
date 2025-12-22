#pragma once

#include "HMIProcessor.h"
#include "Remap/HMIIndexMapping.h"
#include "HMIFaceDetector.generated.h"

USTRUCT(BlueprintType, Category="HMI|FaceDetector")
struct HMI_API FHMIFaceRect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	int X = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="HMI")
	int Y = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="HMI")
	int Width = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="HMI")
	int Height = 0;
};

USTRUCT(BlueprintType, Category="HMI|FaceDetector")
struct HMI_API FHMIFaceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	float Confidence = 0;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	FHMIFaceRect Rect;
	
	UPROPERTY(BlueprintReadOnly, Category="HMI")
	TObjectPtr<UHMIIndexMapping> ExpressionMapping;

	UPROPERTY(BlueprintReadOnly, Category="HMI")
	TArray<float> Expressions;

	float GetExpression(int Id) const { return HMI_GetValue(Expressions, Id); }
	float GetExpression(const FName& Name) const { return HMI_GetValue(ExpressionMapping, Expressions, Name); }
	FName GetExpressionName(int Id) const { return HMI_GetName(ExpressionMapping, Id); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHMIOnFaceDetectComplete);

UCLASS(BlueprintType, Abstract, ClassGroup="HMI|FaceDetector")
class HMI_API UHMIFaceDetector : public UHMIProcessor
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnFaceDetectComplete OnFaceDetectComplete;

	UFUNCTION(BlueprintCallable, Category="HMI|FaceDetector")
	virtual void DetectOnce() {}

	UFUNCTION(BlueprintCallable, Category="HMI|FaceDetector")
	virtual void DetectRealtime(bool Enable) {}

	UFUNCTION(BlueprintCallable, Category="HMI|FaceDetector")
	virtual void GetFaces(UPARAM(ref) TArray<FHMIFaceData>& Faces) {} // dont make it const!
};
