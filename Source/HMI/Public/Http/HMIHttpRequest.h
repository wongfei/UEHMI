#pragma once

#include "HMIMinimal.h"
#include "Containers/AnsiString.h"

DECLARE_DELEGATE_TwoParams(FStreamingHttpRequestProgress, const FAnsiString& /*Response*/, const FAnsiString& /*Chunk*/);
DECLARE_DELEGATE_TwoParams(FStreamingHttpRequestComplete, const FAnsiString& /*Response*/, int /*ErrorCode*/);

class HMI_API FHMIHttpRequest
{
	friend class FCurlStatics;

	public:

	FHMIHttpRequest(const FString& InAuthKey = TEXT(""));
	~FHMIHttpRequest();

	FStreamingHttpRequestProgress OnProgress;
	FStreamingHttpRequestComplete OnComplete;

	bool PostAndWait(const FString& Url, const FString& ContentType, const FString& Content);
	void Cancel();
	bool GetCancelFlag() const { return CancelFlag.load(); }

	protected:

	FString AuthKey;

	FCriticalSection CurlMux;
	std::atomic<bool> CancelFlag;
	void* Curl = nullptr;
};
