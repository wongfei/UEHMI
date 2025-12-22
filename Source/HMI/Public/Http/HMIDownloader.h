#pragma once

#include "HMIMinimal.h"

DECLARE_DELEGATE_TwoParams(FHMIDownloadFileComplete, const FString& /*Url*/, bool /*Success*/);
DECLARE_DELEGATE(FHMIDownloadAllComplete);

class HMI_API FHMIDownloader
{
	public:

	FHMIDownloader();
	~FHMIDownloader();

	FHMIDownloadFileComplete OnDownloadFileComplete;
	FHMIDownloadAllComplete OnDownloadAllComplete;

	void SetOutputDir(const FString& Dirpath);
	void Download(const FString& Url);
	void Download(const TArray<FString>& Urls);
	void Cancel();
	
	protected:
	
	void DownloadNext();
	void HandleDownloadComplete(const FString& Url, bool Success);

	FString OutputDirpath;
	TArray<FString> PendingUrls;
	TSharedPtr<class IHttpRequest, ESPMode::ThreadSafe> HttpRequest;
	bool DownloadPending = false;
};
