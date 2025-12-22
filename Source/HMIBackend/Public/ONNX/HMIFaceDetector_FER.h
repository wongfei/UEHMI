#pragma once

#include "Processors/HMIFaceDetector.h"
#include "HMIFaceDetector_FER.generated.h"

UCLASS()
class HMIBACKEND_API UHMIFaceDetector_FER : public UHMIFaceDetector
{
	GENERATED_BODY()

	friend class FFERImpl;

	public:

	static const FName ImplName;

	UFUNCTION(BlueprintCallable, Category="HMI|Backend", meta=(WorldContext="WorldContextObject"))
	static class UHMIFaceDetector* GetOrCreateFaceDetector_FER(UObject* WorldContextObject, 
		FName Name = NAME_None,
		int CaptureDeviceId = 0,
		int CaptureBackendId = 0,
		float MinConfidence = 0.6f
	);

	UHMIFaceDetector_FER(const FObjectInitializer& ObjectInitializer);
	UHMIFaceDetector_FER(FVTableHelper& Helper);
	~UHMIFaceDetector_FER();

	#if HMI_WITH_FER

	public:

	virtual bool IsProcessorImplemented() const override { return true; }
	virtual void DetectOnce() override;
	virtual void DetectRealtime(bool Enable) override;
	virtual void CancelOperation(bool PurgeQueue = false) override;
	virtual void GetFaces(UPARAM(ref) TArray<FHMIFaceData>& Faces) override;

	protected:

	virtual bool Proc_Init() override;
	virtual bool Proc_DoWork(int& QueueLength) override;
	virtual void Proc_PostWorkStats() override;
	virtual void Proc_Release() override;

	TUniquePtr<class FFERImpl> Impl;
	bool IsRealtime = false;

	#endif // HMI_WITH_FER

	UPROPERTY(Transient)
	TObjectPtr<UHMIIndexMapping> ExpressionMapping;
};
