#include "Processors/HMIProcessor.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "TimerManager.h"

class FHMIWorker : public FRunnable
{
	private:

	mutable FCriticalSection ThreadMux;
	mutable FCriticalSection StateMux;

	FString Name;

	EHMIProcessorState ProcState = EHMIProcessorState::Stopped;
	void SetState(EHMIProcessorState NewState) { FScopeLock LOCK(&StateMux); ProcState = NewState; }

	UHMIProcessor* Context = nullptr;
	FEvent* WakeEvent = nullptr;
	FRunnableThread* Thread = nullptr;

	std::atomic<bool> ThreadCreated;
	std::atomic<bool> ThreadExited;
	std::atomic<bool> StopFlag;

	std::atomic<int> QueueLength = 0;
	std::atomic<float> LastOperationDuration = 0.0f;

	public:

	bool IsThreadRunning() const { return ThreadCreated && !ThreadExited; }
	EHMIProcessorState GetState() const { FScopeLock LOCK(&StateMux); return ProcState; }
	int GetQueueLength() const { return QueueLength.load(); }
	float GetLastOperationDuration() const { return LastOperationDuration.load(); }

	FHMIWorker(const FString& InName, UHMIProcessor* InContext) : 
		Name(InName),
		Context(InContext)
	{
		WakeEvent = FPlatformProcess::GetSynchEventFromPool(false); 
		check(WakeEvent);
	}

	virtual ~FHMIWorker() override
	{
		JoinThread();

		if (WakeEvent)
		{
			FPlatformProcess::ReturnSynchEventToPool(WakeEvent);
			WakeEvent = nullptr;
		}
	}

	bool StartOrWake()
	{
		FScopeLock LOCK(&ThreadMux);

		if (Thread)
		{
			if (!ThreadExited)
			{
				WakeUp();
				return true;
			}

			_WaitCloseHandle();
		}

		UE_LOG(LogHMI, Verbose, TEXT("[%s] Start"), *Name);

		ThreadCreated = true;
		ThreadExited = false;
		StopFlag = false;

		FString ThreadName(TEXT("FHMIWorker_"));
		ThreadName.Append(Name);

		Thread = FRunnableThread::Create(this, *ThreadName); 
		if (!Thread)
		{
			ThreadCreated = false;
			UE_LOG(LogHMI, Error, TEXT("FRunnableThread::Create"));
			return false;
		}

		//Thread->SetThreadPriority(EThreadPriority::TPri_BelowNormal);
		return true;
	}

	virtual void Stop() override
	{
		if (IsThreadRunning())
		{
			UE_LOG(LogHMI, Verbose, TEXT("[%s] Stop"), *Name);
		}

		StopFlag = true;
		WakeUp();
	}

	void JoinThread()
	{
		StopFlag = true;
		WakeUp();

		{
			FScopeLock LOCK(&ThreadMux);
			_WaitCloseHandle();
		}
	}

	void _WaitCloseHandle()
	{
		if (Thread)
		{
			UE_LOG(LogHMI, Verbose, TEXT("[%s] Join"), *Name);
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
			ThreadCreated = false;
			UE_LOG(LogHMI, Verbose, TEXT("[%s] Join DONE"), *Name);
		}
	}

	void WakeUp()
	{
		WakeEvent->Trigger();
	}

	virtual uint32 Run() override
	{
		UE_LOG(LogHMI, Verbose, TEXT("[%s] EnterThread %u"), *Name, FPlatformTLS::GetCurrentThreadId());

		Context->OnWorkerThreadEntered();

		try
		{
			// INIT

			UE_LOG(LogHMI, Verbose, TEXT("[%s] Proc_Init"), *Name);
			SetState(EHMIProcessorState::Starting);

			uint64 CyclesStart = FPlatformTime::Cycles64();
			
			const bool InitDone = Context->Proc_Init();
			
			uint64 CyclesEnd = FPlatformTime::Cycles64();
			float Elapsed = (float)FPlatformTime::ToSeconds64(CyclesEnd - CyclesStart);

			if (InitDone) { UE_LOG(LogHMI, Verbose, TEXT("[%s] Proc_Init DONE (Elapsed %f sec)"), *Name, Elapsed); }
			else          { UE_LOG(LogHMI, Error, TEXT("[%s] Proc_Init FAIL"), *Name); }

			// RUN

			if (InitDone)
			{
				UE_LOG(LogHMI, Verbose, TEXT("[%s] DoWork"), *Name);
				SetState(EHMIProcessorState::Busy);

				while (!StopFlag)
				{
					// auto-reset cancel flag before doing work
					Context->CancelFlag = false;

					bool Success = false;
					int NewQueueLength = 0;

					if (!Context->PauseFlag)
					{
						CyclesStart = FPlatformTime::Cycles64();

						Success = Context->Proc_DoWork(NewQueueLength);
						QueueLength = NewQueueLength;

						CyclesEnd = FPlatformTime::Cycles64();
						Elapsed = (float)FPlatformTime::ToMilliseconds64(CyclesEnd - CyclesStart);

						if (Success || Elapsed > LastOperationDuration)
						{
							LastOperationDuration = Elapsed;
							Context->Proc_PostWorkStats();
						}
					}

					if (!NewQueueLength)
					{
						SetState(EHMIProcessorState::Idle);
						WakeEvent->Wait();
						SetState(EHMIProcessorState::Busy);
					}
				}

				UE_LOG(LogHMI, Verbose, TEXT("[%s] DoWork DONE"), *Name);
			}
		}
		catch (const std::exception& ex)
		{
			UE_LOG(LogHMI, Error, TEXT("[%s] EXCEPTION: %s"), *Name, UTF8_TO_TCHAR(ex.what()));
		}
		catch (...)
		{
			UE_LOG(LogHMI, Error, TEXT("[%s] EPIC FAIL"), *Name);
		}

		// RELEASE

		try
		{
			UE_LOG(LogHMI, Verbose, TEXT("[%s] Proc_Release"), *Name);
			SetState(EHMIProcessorState::Stopping);

			Context->CancelOperation(true);
			Context->Proc_Release();

			SetState(EHMIProcessorState::Stopped);
			UE_LOG(LogHMI, Verbose, TEXT("[%s] Proc_Release DONE"), *Name);
		}
		catch (const std::exception& ex)
		{
			UE_LOG(LogHMI, Error, TEXT("[%s] EXCEPTION: %s"), *Name, UTF8_TO_TCHAR(ex.what()));
		}
		catch (...)
		{
			UE_LOG(LogHMI, Error, TEXT("[%s] EPIC FAIL"), *Name);
		}

		UE_LOG(LogHMI, Verbose, TEXT("[%s] LeaveThread %u"), *Name, FPlatformTLS::GetCurrentThreadId());

		Context->OnWorkerThreadExiting();

		ThreadExited = true;
		return 0;
	}
};

UHMIProcessor::UHMIProcessor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UHMIProcessor::UHMIProcessor(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIProcessor::~UHMIProcessor()
{
	if (Worker)
	{
		if (Worker->IsThreadRunning())
		{
			UE_LOG(LogHMI, Error, TEXT("Processor was not stopped before dtor"));
			UE_DEBUG_BREAK();
		}
		Worker.Reset();
	}
}

void UHMIProcessor::OnProcessorCreated()
{
	Worker = MakeUnique<FHMIWorker>(ProcessorName, this);
}

void UHMIProcessor::OnWorkerThreadEntered()
{
}

void UHMIProcessor::OnWorkerThreadExiting()
{
}

void UHMIProcessor::StartOrWakeProcessor()
{
	if (ProcessorName.IsEmpty())
	{
		UE_LOG(LogHMI, Error, TEXT("Starting unnamed HMIProcessor is not allowed"));
		return;
	}

	if (!IsProcessorImplemented())
	{
		UE_LOG(LogHMI, Error, TEXT("Processor not implemented: %s"), *ProcessorName);
		return;
	}

	Worker->StartOrWake();
}

void UHMIProcessor::StopProcessor(bool WaitForCompletion)
{
	CancelOperation(true);
	PauseFlag = false;

	Worker->Stop();

	if (WaitForCompletion)
	{
		Worker->JoinThread();
	}
}

void UHMIProcessor::CancelOperation(bool PurgeQueue)
{
	CancelFlag = true;
}

void UHMIProcessor::PauseProcessor(bool Cancel, bool PurgeQueue)
{
	PauseFlag = true;

	if (Cancel || PurgeQueue)
	{
		CancelOperation(PurgeQueue);
	}
}

void UHMIProcessor::ResumeProcessor()
{
	PauseFlag = false;
}

bool UHMIProcessor::IsProcessorRunning() const
{
	return Worker->IsThreadRunning();
}

EHMIProcessorState UHMIProcessor::GetProcessorState() const
{
	return Worker->GetState();
}

int UHMIProcessor::GetProcessorQueueLength() const
{
	return Worker->GetQueueLength();
}

float UHMIProcessor::GetLastOperationDuration() const
{
	return Worker->GetLastOperationDuration();
}

// Params

void UHMIProcessor::SetProcessorParam(const TMap<FName, FString>& Params)
{
	FScopeLock LOCK(&ProcessorParamsMux);
	for (const auto& Iter : Params)
		ProcessorParams.Add({ Iter.Key, Iter.Value });
}

void UHMIProcessor::SetProcessorParam(FName Key, const FString& Value)
{
	FScopeLock LOCK(&ProcessorParamsMux);
	ProcessorParams.Add(Key, Value);
}

void UHMIProcessor::SetProcessorParam(FName Key, float Value)
{
	SetProcessorParam(Key, LexToString(Value));
}

void UHMIProcessor::SetProcessorParam(FName Key, int Value)
{
	SetProcessorParam(Key, LexToString(Value));
}

void UHMIProcessor::SetProcessorParam(FName Key, bool Value)
{
	SetProcessorParam(Key, LexToString(Value));
}

FString UHMIProcessor::GetProcessorString(FName Key) const
{
	FScopeLock LOCK(&ProcessorParamsMux);
	return ProcessorParams.FindRef(Key); 
}

float UHMIProcessor::GetProcessorFloat(FName Key) const
{
	return FCString::Atof(*GetProcessorString(Key));
}

int32 UHMIProcessor::GetProcessorInt(FName Key) const
{
	return FCString::Atoi(*GetProcessorString(Key));
}

bool UHMIProcessor::GetProcessorBool(FName Key) const
{
	return FCString::ToBool(*GetProcessorString(Key));
}

// Error

void UHMIProcessor::ProcError(const FString& ErrorInfo)
{
	TraceError(ErrorInfo);
	BroadcastError(ErrorInfo);
}

void UHMIProcessor::TraceError(const FString& ErrorInfo)
{
	#if !UE_BUILD_SHIPPING

	// dirty hack to ignore empty input errors
	if (ErrorInfo.Contains(TEXT("Empty input")))
		return;

	const FString Msg = FString::Printf(TEXT("[%s] %s"), *ProcessorName, *ErrorInfo);
	UE_LOG(LogHMI, Error, TEXT("%s"), *Msg);

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, Msg);

	#endif
}

void UHMIProcessor::BroadcastError(const FString& ErrorInfo)
{
	if (IsInGameThread())
	{
		OnProcessorError.Broadcast(ErrorInfo);
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, ErrorInfo]()
		{
			OnProcessorError.Broadcast(ErrorInfo);
		});
	}
}

// Misc

class UObject* UHMIProcessor::GetWorldContext() const
{
	return GameInstance;
}

class FTimerManager* UHMIProcessor::GetWorldTimerManager() const
{
	if (GameInstance && GameInstance->GetWorld())
		return &GameInstance->GetWorld()->GetTimerManager();
	return nullptr;
}

int64 UHMIProcessor::GenerateId() const
{
	return UHMIStatics::GenerateOperationId();
}

double UHMIProcessor::GetTimestamp() const
{
	return UHMIStatics::GetTimestamp(GameInstance);
}

int UHMIProcessor::GetNumBackendThreads() const
{
	const int NumCores = FPlatformMisc::NumberOfCores();
	const int NumThreads = GetProcessorInt("NumThreads");
	
	if (NumThreads == -1)
		return NumCores;

	return FMath::Clamp(NumThreads, 0, NumCores * 2);
}
