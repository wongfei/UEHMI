#pragma once

#include "HMIStatics.h"

template<typename TOperation, typename TInput>
class THMIProcessorQueue
{
	public:

	THMIProcessorQueue() {}

	protected:

	FCriticalSection InputQueueMux;

	int64 EnqueOperation(TArray<TOperation>& Queue, TInput&& Input, UObject* WorldContext)
	{
		const int64 OperationId = UHMIStatics::GenerateOperationId();
		//UE_LOG(LogHMI, VeryVerbose, TEXT("EnqueOperation: %lld"), OperationId);

		Input.OperationId = OperationId;
		Input.Timestamp = UHMIStatics::GetTimestamp(WorldContext);

		{
			FScopeLock LOCK(&InputQueueMux);
			Queue.Add(TOperation(MoveTemp(Input)));
		}

		return OperationId;
	}

	bool DequeOperation(TArray<TOperation>& Queue, TOperation& Operation, int& QueueLength)
	{
		{
			FScopeLock LOCK(&InputQueueMux);
			if (Queue.IsEmpty())
			{
				QueueLength = 0;
				return false;
			}

			Operation = MoveTemp(Queue[0]);
			Queue.RemoveAt(0, EAllowShrinking::No);
			QueueLength = Queue.Num();
		}

		//UE_LOG(LogHMI, VeryVerbose, TEXT("DequeOperation: %lld"), Operation.Input.OperationId);
		return true;
	}

	void PurgeInputQueue(TArray<TOperation>& Queue)
	{
		{
			FScopeLock LOCK(&InputQueueMux);
			Queue.Empty();
		}
	}
};
