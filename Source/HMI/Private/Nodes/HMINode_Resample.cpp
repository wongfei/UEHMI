#include "HMINode_Resample.h"
#include "HMISubsystem.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

UHMINode_Resample* UHMINode_Resample::Resample_Async(UObject* WorldContextObject, FHMIWaveHandle Wave, int OutputSampleRate)
{
	UHMINode_Resample* Node = NewObject<UHMINode_Resample>();
	Node->WorldContext = WorldContextObject;
	Node->Wave = MoveTemp(Wave);
	Node->OutputSampleRate = OutputSampleRate;
	return Node;
}

void UHMINode_Resample::Activate()
{
	if (Complete.IsBound())
	{
		if (OutputSampleRate == 0)
		{
			OutputSampleRate = HMI_DEFAULT_SAMPLE_RATE;
		}

		if (!UHMIBufferStatics::IsValidSampleRate(OutputSampleRate))
		{
			UE_LOG(LogHMI, Error, TEXT("Resample_Async: Invalid OutputSampleRate: %d"), OutputSampleRate);
			return;
		}

		if (Wave.IsEmpty() || Wave.GetSampleRate() == OutputSampleRate)
		{
			FHMIResampleResult Result{ MoveTemp(Wave), OutputSampleRate };
			Complete.Broadcast(Result);
			return;
		}

		UHMISubsystem* HMI = UHMISubsystem::GetInstance(WorldContext);
		if (ensure(HMI))
		{
			RegisterWithGameInstance(WorldContext);

			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, HMI]()
			{
				FHMIWaveHandle OutWave = HMI->Resample(Wave, OutputSampleRate); // blocking!
				FHMIResampleResult Result{ MoveTemp(OutWave), OutputSampleRate };

				AsyncTask(ENamedThreads::GameThread, [this, Result = MoveTemp(Result)]() mutable
				{
					Complete.Broadcast(Result);
					SetReadyToDestroy();
				});
			});
		}
	}
}
