#include "Cloud/HMIChat_OpenAI.h"
#include "HMIProcessorImpl.h"
#include "HMISubsystemStatics.h"

#include "Http/HMIHttpRequest.h"
#include "JsonObjectConverter.h"

// https://platform.openai.com/docs/api-reference/chat/create

HMI_PROC_DECLARE_STATS(Chat_OpenAI)

#define LOGPREFIX "[Chat_OpenAI] "

const FName UHMIChat_OpenAI::ImplName = TEXT("Chat_OpenAI");

class UHMIChat* UHMIChat_OpenAI::GetOrCreateChat_OpenAI(UObject* WorldContextObject, 
	FName Name,
	FString ModelName,
	FString BackendUrl,
	FString BackendKey
)
{
	UHMIChat* Proc = UHMISubsystemStatics::GetOrCreateChat(WorldContextObject, ImplName, Name);

	if (BackendUrl.IsEmpty())
	{
		BackendUrl = TEXT("http://127.0.0.1:8080/v1/chat/completions");
		//BackendUrl = TEXT("https://api.openai.com/v1/chat/completions");
	}

	Proc->SetProcessorParam("ModelName", ModelName);
	Proc->SetProcessorParam("BackendUrl", BackendUrl);
	Proc->SetProcessorParam("BackendKey", BackendKey);

	return Proc;
}

UHMIChat_OpenAI::UHMIChat_OpenAI(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProviderName = ImplName;
	ProcessorName = ImplName.ToString();

	#if 1
		SetProcessorParam("ModelName", TEXT("Llama-3.2_1b_Uncensored_RP_Aesir.gguf"));
		SetProcessorParam("BackendUrl", TEXT("http://127.0.0.1:8080/v1/chat/completions"));
		
	#else
		SetProcessorParam("ModelName", TEXT("gpt-5-nano"));
		SetProcessorParam("BackendUrl", TEXT("https://api.openai.com/v1/chat/completions"));
		SetProcessorParam("BackendKey", TEXT(""));
	#endif

	// api.openai.com

	// What sampling temperature to use, between 0 and 2. Higher values like 0.8 will make the output more random, while lower values like 0.2 will make it more focused and deterministic. We generally recommend altering this or top_p but not both.
	// temperature = 1.0

	// An alternative to sampling with temperature, called nucleus sampling, where the model considers the results of the tokens with top_p probability mass. So 0.1 means only the tokens comprising the top 10% probability mass are considered.
	// top_p = 1.0

	// Number between -2.0 and 2.0. Positive values penalize new tokens based on their existing frequency in the text so far, decreasing the model's likelihood to repeat the same line verbatim.
	// frequency_penalty = 0.0

	// Number between -2.0 and 2.0. Positive values penalize new tokens based on whether they appear in the text so far, increasing the model's likelihood to talk about new topics.
	// presence_penalty = 0.0

	// Constrains effort on reasoning for reasoning models. Currently supported values are none, minimal, low, medium, and high.
	// reasoning_effort = "none"

	// How many chat completion choices to generate for each input message. Note that you will be charged based on the number of generated tokens across all of the choices. Keep n as 1 to minimize costs.
	// n = 1
}

UHMIChat_OpenAI::UHMIChat_OpenAI(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIChat_OpenAI::~UHMIChat_OpenAI()
{
}

#if HMI_WITH_OPENAI_CHAT

bool UHMIChat_OpenAI::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	BackendUrl = GetProcessorString("BackendUrl");
	BackendKey = GetProcessorString("BackendKey");
	HMI_GUARD_PARAM_NOT_EMPTY(BackendUrl);

	ModelName = GetProcessorString("ModelName");
	HMI_GUARD_PARAM_NOT_EMPTY(ModelName);

	HttpRequest = MakeUnique<FHMIHttpRequest>(BackendKey);
	HttpRequest->OnProgress.BindUObject(this, &UHMIChat_OpenAI::OnHttpRequestProgress);
	HttpRequest->OnComplete.BindUObject(this, &UHMIChat_OpenAI::OnHttpRequestComplete);

	return true;
}

void UHMIChat_OpenAI::Proc_Release()
{
	HttpRequest.Reset();

	Super::Proc_Release();
}

int64 UHMIChat_OpenAI::ProcessInput(FHMIChatInput&& Input)
{
	const int64 OperationId = EnqueOperation(InputQueue, MoveTemp(Input), GetWorldContext());
	StartOrWakeProcessor();
	return OperationId;
}

void UHMIChat_OpenAI::CancelOperation(bool PurgeQueue)
{
	Super::CancelOperation(PurgeQueue);

	if (HttpRequest)
		HttpRequest->Cancel();

	if (PurgeQueue)
		PurgeInputQueue(InputQueue);
}

bool UHMIChat_OpenAI::Proc_DoWork(int& QueueLength)
{
	if (!DequeOperation(InputQueue, Operation, QueueLength))
		return false;

	auto& InputText = Operation.Input.Text;

	if (InputText.IsEmpty())
	{
		HandleOperationComplete(false, FString(TEXT("Empty input")), MoveTemp(Operation.Input));
		return false;
	}

	HMI_PROC_PRE_WORK_STATS(Chat_OpenAI)

	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "<<< \"%s\""), *InputText);

	ResetChatOperation();
	ErrorText.Reset();
	DataChunkPos = 0;
	BadChunkCount = 0;

	TArray<TSharedPtr<FJsonValue>> JsonMessages;

	for (const auto& Msg : Operation.Input.History)
	{
		TSharedRef<FJsonObject> JsonMsg = MakeShared<FJsonObject>();
		JsonMsg->SetStringField(TEXT("role"), Msg.Role);
		JsonMsg->SetStringField(TEXT("content"), Msg.Text);
		JsonMessages.Add(MakeShared<FJsonValueObject>(JsonMsg));
	}

	TSharedRef<FJsonObject> JsonMsg = MakeShared<FJsonObject>();
	JsonMsg->SetStringField(TEXT("role"), TEXT("user"));
	JsonMsg->SetStringField(TEXT("content"), InputText);
	JsonMessages.Add(MakeShared<FJsonValueObject>(JsonMsg));

	// root node
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetStringField(TEXT("model"), ModelName);
	JsonObject->SetBoolField(TEXT("stream"), true);
	JsonObject->SetArrayField(TEXT("messages"), JsonMessages);

	// backend params
	if (BackendUrl.Contains(TEXT("api.openai.com")))
	{
		JsonObject->SetStringField(TEXT("max_completion_tokens"), LexToString(Operation.Input.MaxTokens));
	}
	else
	{
		JsonObject->SetStringField(TEXT("max_tokens"), LexToString(Operation.Input.MaxTokens));
	}

	JsonObject->SetStringField(TEXT("temperature"), LexToString(Operation.Input.Temperature));
	JsonObject->SetStringField(TEXT("top_p"), LexToString(Operation.Input.TopP));

	for (const auto& Iter : Operation.Input.BackendParams)
	{
		JsonObject->SetStringField(Iter.Key, Iter.Value);
	}

	FString Content;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Content);
	FJsonSerializer::Serialize(JsonObject, Writer);

	const bool Success = HttpRequest->PostAndWait(BackendUrl, TEXT("application/json"), Content);

	if (!HttpRequest->GetCancelFlag())
	{
		HandleOperationComplete(Success, MoveTemp(ErrorText), MoveTemp(Operation.Input));
	}

	return Success;
}

void UHMIChat_OpenAI::Proc_PostWorkStats()
{
	HMI_PROC_POST_WORK_STATS(Chat_OpenAI)
}

void UHMIChat_OpenAI::OnHttpRequestProgress(const FAnsiString& ResponseAnsi, const FAnsiString& Chunk)
{
	// data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"XXX"}}],"created":1760371448,"id":"chatcmpl-asie3B4BpRtih8UlmoetL8rvFDttgTIk","model":"Llama-3.2-3B","system_fingerprint":"b6745-a31cf36a","object":"chat.completion.chunk"}\n\n
	// data: {...}
	// data: [DONE]

	static const FAnsiStringView DataPrefix(("data:"));
	static const FAnsiStringView EndChunk(("\n\n"));

	const int ResponseLen = ResponseAnsi.Len();
	while (DataChunkPos < ResponseLen)
	{
		FAnsiStringView StreamView(&ResponseAnsi[DataChunkPos], ResponseLen - DataChunkPos);

		const int32 DataIndex = StreamView.Find(DataPrefix);
		if (DataIndex == INDEX_NONE) // no data
		{
			DataChunkPos = ResponseLen; // skip whole chunk
			break;
		}

		if (DataIndex > 0) // data in the middle
		{
			DataChunkPos += DataIndex; // seek to start
			continue;
		}

		const int32 EndChunkIndex = StreamView.Find(EndChunk);
		if (EndChunkIndex == INDEX_NONE)
		{
			break;
		}

		DataChunkPos += (EndChunkIndex + EndChunk.Len());

		{
			const int32 ChunkBeginIndex = DataIndex + DataPrefix.Len();
			FAnsiStringView Payload(&StreamView[ChunkBeginIndex], EndChunkIndex - ChunkBeginIndex);

			int32 BracketIndex;
			if (Payload.FindChar(('{'), BracketIndex))
			{
				const FUTF8ToTCHAR Conv(Payload.GetData(), Payload.Len());
				FString JsonString(Conv);

				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::CreateFromView(JsonString);
				TSharedPtr<FJsonObject> JsonObject;
				if (!FJsonSerializer::Deserialize(Reader, JsonObject))
				{
					//ErrorText = TEXT("Deserialize");
					++BadChunkCount;
				}
				else
				{
					FOpenAIChatCompletionResponse OpenAIResponse;
					if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &OpenAIResponse))
					{
						//ErrorText = TEXT("JsonObjectToUStruct");
						++BadChunkCount;
					}
					else
					{
						if (!OpenAIResponse.error.code.IsEmpty() || !OpenAIResponse.error.message.IsEmpty())
						{
							ErrorText = FString::Printf(TEXT(LOGPREFIX "Backend error: code=%s message=%s"), *OpenAIResponse.error.code, *OpenAIResponse.error.message);
							++BadChunkCount;
						}
						else
						{
							if (!OpenAIResponse.choices.IsEmpty() && !OpenAIResponse.choices[0].delta.content.IsEmpty())
							{
								HandleDeltaContent(Operation.Input, MoveTemp(OpenAIResponse.choices[0].delta.content));
							}
						}
					}
				}
			}
		}
	} 
}

void UHMIChat_OpenAI::OnHttpRequestComplete(const FAnsiString& ResponseAnsi, int ErrorCode)
{
	if (ErrorCode != 0)
	{
		ErrorText = FString::Printf(TEXT("CURL: %d"), ErrorCode);
		return;
	}

	if (ResponseAnsi.Contains(TEXT("\"error\":")))
	{
		const FUTF8ToTCHAR Conv(ResponseAnsi.GetCharArray().GetData(), ResponseAnsi.GetCharArray().Num());
		FString Response(Conv);

		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::CreateFromView(Response);
		TSharedPtr<FJsonObject> JsonObject;
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			FOpenAIChatCompletionResponse OpenAIResponse;
			if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &OpenAIResponse))
			{
				ErrorText = FString::Printf(TEXT("Backend error: code=%s message=%s"), *OpenAIResponse.error.code, *OpenAIResponse.error.message);
				return;
			}
		}

		ErrorText = TEXT("Unknown backend error");
		return;
	}

	if (BadChunkCount > 0 && OutputMsg.IsEmpty() && ErrorText.IsEmpty())
	{
		ErrorText = TEXT("Stream error");
		return;
	}
}

#endif // HMI_WITH_OPENAI_CHAT

#undef LOGPREFIX
