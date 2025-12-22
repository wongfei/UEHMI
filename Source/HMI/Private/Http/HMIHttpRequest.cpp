#include "Http/HMIHttpRequest.h"

#include "curl/curl.h"
#include <string>

class FCurlStatics
{
	public:

	static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);
	static size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
	static int ProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
};

class FHMIHttpRequestContext
{
	public:

	FHMIHttpRequest* Impl;
	FAnsiString Chunk;
	FAnsiString Response;
	bool IsUtf8 = false;

	FHMIHttpRequestContext(FHMIHttpRequest* InImpl) : Impl(InImpl)
	{
		Chunk.Reserve(512);
		Response.Reserve(4096);
	}

	void AppendResponse(const char* str, int count)
	{
		Chunk.Reset();
		if (count > 0)
		{
			Chunk.Append((const ANSICHAR*)str, count);
			Response.Append(Chunk);
		}
	}
};

FHMIHttpRequest::FHMIHttpRequest(const FString& InAuthKey) : AuthKey(InAuthKey)
{
	{
		FScopeLock LOCK(&CurlMux);
		Curl = curl_easy_init();
		check(Curl);
	}
}

FHMIHttpRequest::~FHMIHttpRequest()
{
	CancelFlag = true;
	{
		FScopeLock LOCK(&CurlMux);
		curl_easy_cleanup(Curl);
	}
}

bool FHMIHttpRequest::PostAndWait(const FString& Url, const FString& ContentType, const FString& Content)
{
	FHMIHttpRequestContext Context(this);

	struct curl_slist* Headers = nullptr;
	std::string TmpStr;

	if (!ContentType.IsEmpty())
	{
		// "Content-Type: application/json; charset=utf-8"
		TmpStr = "Content-Type: ";
		TmpStr.append(TCHAR_TO_ANSI(*ContentType));
		TmpStr.append("; charset=utf-8");
		Headers = curl_slist_append(Headers, TmpStr.c_str());
	}

	if (!AuthKey.IsEmpty())
	{
		// "Authorization: Bearer XXXX"
		TmpStr = "Authorization: Bearer ";
		TmpStr.append(TCHAR_TO_ANSI(*AuthKey));
		Headers = curl_slist_append(Headers, TmpStr.c_str());
	}

	CURLcode ErrorCode;
	{
		FScopeLock LOCK(&CurlMux);
		curl_easy_reset(Curl);

		curl_easy_setopt(Curl, CURLOPT_URL, TCHAR_TO_ANSI(*Url));
		curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);

		FTCHARToUTF8 Utf8Converter(*Content);
		curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, Utf8Converter.Get()); // data not copied!
		curl_easy_setopt(Curl, CURLOPT_POSTFIELDSIZE, Utf8Converter.Length());

		curl_easy_setopt(Curl, CURLOPT_HEADERFUNCTION, FCurlStatics::HeaderCallback);
		curl_easy_setopt(Curl, CURLOPT_HEADERDATA, &Context);
		curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, FCurlStatics::WriteCallback);
		curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Context);

		curl_easy_setopt(Curl, CURLOPT_XFERINFODATA, &Context);
		curl_easy_setopt(Curl, CURLOPT_XFERINFOFUNCTION, FCurlStatics::ProgressCallback);
		curl_easy_setopt(Curl, CURLOPT_NOPROGRESS, 0L);

		curl_easy_setopt(Curl, CURLOPT_CONNECTTIMEOUT, 60L); // default 300 seconds
		curl_easy_setopt(Curl, CURLOPT_TIMEOUT, 0L);
		curl_easy_setopt(Curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYHOST, 0L);

		UE_LOG(LogHMI, Verbose, TEXT("curl_easy_perform"));

		CancelFlag = false;
		ErrorCode = curl_easy_perform(Curl);
	}

	if (ErrorCode != CURLE_OK)
	{
		UE_LOG(LogHMI, Error, TEXT("curl_easy_perform: err=%d"), ErrorCode);
	}

	if (Headers)
	{
		curl_slist_free_all(Headers);
	}

	if (CancelFlag)
	{
		Context.Response.Reset();
	}
	else
	{
		OnComplete.ExecuteIfBound(Context.Response, (int)ErrorCode);
	}

	return ErrorCode == CURLE_OK && !CancelFlag;
}

void FHMIHttpRequest::Cancel()
{
	CancelFlag = true;
}

size_t FCurlStatics::HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	auto* Context = (FHMIHttpRequestContext*)userdata;
	const size_t TotalSize = size * nitems;

	#if 1
	const FAnsiStringView Header(buffer, nitems);

	if (Header.Contains("Content-Type", ESearchCase::IgnoreCase)
		&& Header.Contains("charset=utf-8", ESearchCase::IgnoreCase))
	{
		Context->IsUtf8 = true;
	}
	#endif

	return TotalSize;
}

size_t FCurlStatics::WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	auto* Context = (FHMIHttpRequestContext*)userdata;
	const size_t TotalSize = size * nmemb;

	if (TotalSize > 0)
	{
		Context->AppendResponse(ptr, nmemb);
		Context->Impl->OnProgress.ExecuteIfBound(Context->Response, Context->Chunk);
	}

	return TotalSize;
}

int FCurlStatics::ProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	auto* Context = (FHMIHttpRequestContext*)clientp;
	return Context->Impl->CancelFlag ? 1 : 0;
}
