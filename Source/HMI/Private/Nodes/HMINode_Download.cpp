#include "HMINode_Download.h"
#include "HMISubsystem.h"
#include "Http/HMIDownloader.h"

UHMINode_Download* UHMINode_Download::Download_Async(UObject* WorldContextObject, TArray<FString> Urls)
{
	UHMINode_Download* Node = NewObject<UHMINode_Download>();
	Node->WorldContext = WorldContextObject;
	Node->Urls = MoveTemp(Urls);
	return Node;
}

void UHMINode_Download::Activate()
{
	if (Complete.IsBound())
	{
		if (Urls.IsEmpty())
		{
			Complete.Broadcast();
			return;
		}

		UHMISubsystem* HMI = UHMISubsystem::GetInstance(WorldContext);
		if (ensure(HMI))
		{
			auto Downloader = HMI->GetDownloader();
			if (ensure(Downloader))
			{
				Downloader->OnDownloadAllComplete.BindLambda([this]()
				{
					Complete.Broadcast();
					SetReadyToDestroy();
				});

				RegisterWithGameInstance(WorldContext);
				Downloader->Download(Urls);
			}
		}
	}
}
