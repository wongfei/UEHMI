#pragma once

#include "HMIBuffer.h"
#include "HMIStatics.h"

#include "HMIProcessorType.h"
#include "HMIProcessorState.h"
#include "HMIProcessorInput.h"
#include "HMIProcessorResult.h"
#include "HMIProcessorQueue.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"
#include "Misc/Paths.h"

#include "HMIProcessor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHMIOnProcessorError, const FString&, ErrorInfo);

UCLASS(BlueprintType, Abstract, ClassGroup="HMI|Processor")
class HMI_API UHMIProcessor : public UObject
{
	GENERATED_BODY()

	friend class UHMISubsystem;
	friend class FHMIWorker;

	public:

	UHMIProcessor(const FObjectInitializer& ObjectInitializer);
	UHMIProcessor(FVTableHelper& Helper);
	~UHMIProcessor();

	virtual bool IsProcessorImplemented() const { return false; }

	// Start Stop

	UPROPERTY(BlueprintAssignable, Category="HMI|Processor")
	FHMIOnProcessorError OnProcessorError;

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void StartOrWakeProcessor();

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void StopProcessor(bool WaitForCompletion = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void CancelOperation(bool PurgeQueue = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void PauseProcessor(bool Cancel = false, bool PurgeQueue = false);

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void ResumeProcessor();

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual bool IsProcessorRunning() const;

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual EHMIProcessorState GetProcessorState() const;

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual int GetProcessorQueueLength() const;

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual float GetLastOperationDuration() const; // milliseconds

	// Params

	virtual void SetProcessorParam(const TMap<FName, FString>& Value);
	virtual void SetProcessorParam(FName Key, const FString& Value);
	virtual void SetProcessorParam(FName Key, const TCHAR* Value) { SetProcessorParam(Key, FString(Value)); }
	virtual void SetProcessorParam(FName Key, float Value);
	virtual void SetProcessorParam(FName Key, int Value);
	virtual void SetProcessorParam(FName Key, bool Value);

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void SetProcessorParams(const TMap<FName, FString>& Params) { SetProcessorParam(Params); }

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void SetProcessorString(FName Key, const FString& Value) { SetProcessorParam(Key, Value); }

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void SetProcessorFloat(FName Key, float Value) { SetProcessorParam(Key, Value); }

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void SetProcessorInt(FName Key, int Value) { SetProcessorParam(Key, Value); }

	UFUNCTION(BlueprintCallable, Category="HMI|Processor")
	virtual void SetProcessorBool(FName Key, bool Value) { SetProcessorParam(Key, Value); }

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual FString GetProcessorString(FName Key) const;

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual float GetProcessorFloat(FName Key) const;

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual int32 GetProcessorInt(FName Key) const;

	UFUNCTION(BlueprintPure, Category="HMI|Processor")
	virtual bool GetProcessorBool(FName Key) const;

	// Misc

	class UGameInstance* GetGameInstance() const { return GameInstance; }
	class UObject* GetWorldContext() const;
	class FTimerManager* GetWorldTimerManager() const;
	int64 GenerateId() const;
	double GetTimestamp() const;
	int GetNumBackendThreads() const;

	// Internals

	protected:

	virtual void OnProcessorCreated();
	virtual void OnWorkerThreadEntered();
	virtual void OnWorkerThreadExiting();

	virtual bool Proc_Init() { return true; }
	virtual bool Proc_DoWork(int& QueueLength) { return false; }
	virtual void Proc_PostWorkStats() {}
	virtual void Proc_Release() {}

	// Result

	template<typename TInput, typename TResult, typename TDelegate>
	void BroadcastResult(TInput&& Input, TResult&& Result, TDelegate& Complete)
	{
		if (!Result.Success && !Result.Error.IsEmpty())
		{
			TraceError(Result.Error);
		}

		if (Input.CompleteFunc)
		{
			Input.CompleteFunc(Input, Result);
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [Proc = this, Input = MoveTemp(Input), Result = MoveTemp(Result), &Complete]() mutable
			{
				if (IsValid(Proc))
				{
					Complete.Broadcast(Input, Result);
				}
			});
		}
	}

	// Error

	void ProcError(const FString& ErrorInfo);
	void TraceError(const FString& ErrorInfo);
	void BroadcastError(const FString& ErrorInfo);

	protected:

	FName ProviderName;
	FString ProcessorName;

	mutable FCriticalSection ProcessorParamsMux;
	TMap<FName, FString> ProcessorParams;

	UPROPERTY(Transient)
	TObjectPtr<class UHMISubsystem> HMISubsystem;

	UPROPERTY(Transient)
	TObjectPtr<class UGameInstance> GameInstance;

	TUniquePtr<class FHMIWorker> Worker;

	std::atomic<bool> CancelFlag;
	std::atomic<bool> PauseFlag;
};
