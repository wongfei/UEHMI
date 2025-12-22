#include "Http/HMIDownloader.h"
#include "HMIStatics.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Async/Async.h"
#include "Async/TaskGraphInterfaces.h"

FHMIDownloader::FHMIDownloader()
{
	OutputDirpath = UHMIStatics::GetHMIDataDir();
}

FHMIDownloader::~FHMIDownloader()
{
	Cancel();
}

void FHMIDownloader::SetOutputDir(const FString& Dirpath)
{
	OutputDirpath = Dirpath;
}

void FHMIDownloader::Download(const FString& Url)
{
	PendingUrls.Add(Url);
	DownloadNext();
}

void FHMIDownloader::Download(const TArray<FString>& Urls)
{
	PendingUrls.Append(Urls);
	DownloadNext();
}

void FHMIDownloader::Cancel()
{
	PendingUrls.Reset();
	if (HttpRequest && DownloadPending)
	{
		HttpRequest->CancelRequest();
		DownloadPending = false;
	}
}

void FHMIDownloader::DownloadNext()
{
	if (DownloadPending || PendingUrls.IsEmpty())
		return;

	FString Url = MoveTemp(PendingUrls[0]);
	PendingUrls.RemoveAt(0, EAllowShrinking::No);

	int32 SlashIdx = -1;
	if (!Url.FindLastChar(TEXT('/'), SlashIdx) || SlashIdx <= 0)
	{
		UE_LOG(LogHMI, Error, TEXT("Download: Invalid url: %s"), *Url);
		HandleDownloadComplete(Url, false);
		return;
	}

	FString Filename = Url.RightChop(SlashIdx + 1);
	if (Filename.IsEmpty())
	{
		UE_LOG(LogHMI, Error, TEXT("Download: Invalid url: %s"), *Url);
		HandleDownloadComplete(Url, false);
		return;
	}

	if (OutputDirpath.IsEmpty())
	{
		UE_LOG(LogHMI, Error, TEXT("Download: Invalid output dir"));
		HandleDownloadComplete(Url, false);
		return;
	}

	if (!FPaths::DirectoryExists(OutputDirpath))
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.CreateDirectory(*OutputDirpath))
		{
			UE_LOG(LogHMI, Error, TEXT("CreateDirectory failed: %s"), *OutputDirpath);
			HandleDownloadComplete(Url, false);
			return;
		}
	}

	FString Filepath = OutputDirpath / Filename;
	if (FPaths::FileExists(Filepath))
	{
		UE_LOG(LogHMI, Verbose, TEXT("Download: Already exists: %s"), *Url);
		HandleDownloadComplete(Url, true);
		return;
	}

	if (!HttpRequest)
		HttpRequest = FHttpModule::Get().CreateRequest().ToSharedPtr();

    HttpRequest->SetVerb("GET");
    HttpRequest->SetURL(Url);

    HttpRequest->OnProcessRequestComplete().BindLambda(
		[this, Url = FString(Url), Filepath = MoveTemp(Filepath)]
		(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) mutable
    {
		DownloadPending = false;
        if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()) && Response->GetContent().Num() > 0)
        {
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, 
				[this, Response, Url = FString(Url), Filepath = MoveTemp(Filepath)]() mutable
			{
				const TArray<uint8>& Content = Response->GetContent();
				if (FFileHelper::SaveArrayToFile(Content, *Filepath))
				{
					UE_LOG(LogHMI, Verbose, TEXT("Download complete: %s"), *Filepath);
					AsyncTask(ENamedThreads::GameThread, [this, Url = FString(Url)]()
					{
						HandleDownloadComplete(Url, true);
					});
				}
				else
				{
					UE_LOG(LogHMI, Error, TEXT("SaveArrayToFile: %s"), *Filepath);
					AsyncTask(ENamedThreads::GameThread, [this, Url = FString(Url)]()
					{
						HandleDownloadComplete(Url, false);
					});
				}
			});
        }
        else
        {
            UE_LOG(LogHMI, Error, TEXT("Download failed: Code=%d"), Response.IsValid() ? Response->GetResponseCode() : 0);
			HandleDownloadComplete(Url, false);
        }
    });

	UE_LOG(LogHMI, Verbose, TEXT("Download: %s"), *Url);
	DownloadPending = true;
    HttpRequest->ProcessRequest();
}

void FHMIDownloader::HandleDownloadComplete(const FString& Url, bool Success)
{
	if (OnDownloadFileComplete.IsBound())
	{
		OnDownloadFileComplete.Execute(Url, Success);
	}

	if (PendingUrls.IsEmpty())
	{
		//UE_LOG(LogHMI, Verbose, TEXT("DownloadAllComplete"));

		if (OnDownloadAllComplete.IsBound())
		{
			OnDownloadAllComplete.Execute();
		}
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			DownloadNext();
		});
	}
}
