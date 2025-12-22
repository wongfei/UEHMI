#include "Processors/HMIChat.h"

#define LOGPREFIX "[Chat] "

static int32 HMIChatLogDelta = 0;
static FAutoConsoleVariableRef CVar_HMIChatLogDelta(TEXT("hmi.chat.LogDelta"), HMIChatLogDelta, TEXT(""), ECVF_Default);

UHMIChat::UHMIChat(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	OutputMsg.Reserve(4096);

	SetProcessorParam("BackendUrl", TEXT(""));
	SetProcessorParam("BackendKey", TEXT(""));
	SetProcessorParam("ModelName", TEXT(""));

	SetProcessorParam("EnableBatching", true);
	SetProcessorParam("MinBatchLength", 5);
	SetProcessorParam("MaxBatchLength", 200);
	SetProcessorParam("SpaceSplitThreshold", 150);
	SetProcessorParam("BatchSplitCharacters", TEXT(".!?;"));
}

UHMIChat::UHMIChat(FVTableHelper& Helper) : Super(Helper)
{
}

UHMIChat::~UHMIChat()
{
}

void UHMIChat::SetupBatching(bool Enable, int MinLen, int MaxLen, int SpaceSplitThresh, FString SplitCharacters)
{
	SetProcessorParam("EnableBatching", Enable);
	SetProcessorParam("MinBatchLength", MinLen);
	SetProcessorParam("MaxBatchLength", MaxLen);
	SetProcessorParam("SpaceSplitThreshold", SpaceSplitThresh);
	SetProcessorParam("BatchSplitCharacters", SplitCharacters);
}

bool UHMIChat::Proc_Init()
{
	if (!Super::Proc_Init())
		return false;

	EnableBatching = GetProcessorBool("EnableBatching");
	MinBatchLength = FMath::Clamp(GetProcessorInt("MinBatchLength"), 0, 200);
	MaxBatchLength = FMath::Clamp(GetProcessorInt("MaxBatchLength"), MinBatchLength + 1, 1000);
	SpaceSplitThreshold = FMath::Clamp(GetProcessorInt("SpaceSplitThreshold"), MinBatchLength + 1, MaxBatchLength);
	BatchSplitCharacters = GetProcessorString("BatchSplitCharacters");

	return true;
}

void UHMIChat::ResetChatOperation()
{
	OutputMsg.Reset();
	ThinkStartIdx = INDEX_NONE;
	ThinkEndIdx = INDEX_NONE;
	ThinkScanOffset = 0;
	ThinkFlag = false;
	BatchPos = 0;
}

void UHMIChat::HandleDeltaContent(const FHMIChatInput& Input, FString&& Delta)
{
	UHMIStatics::RemoveSpecialCharacters(Delta);
	OutputMsg.Append(Delta);
	BroadcastChatDelta(Input, MoveTemp(Delta), false);

	DetectThinkTags(Input);

	if (EnableBatching)
	{
		AccumulateBatch(Input);
	}
}

static const FString ThinkStartStr(TEXT("<think>"));
static const FString ThinkEndStr(TEXT("</think>"));

// you can add /think and /no_think to user prompts or system messages to switch the model's thinking mode from turn to turn
void UHMIChat::DetectThinkTags(const FHMIChatInput& Input)
{
	if (ThinkScanOffset >= OutputMsg.Len())
		return;

	bool TagFound = false;

	if (ThinkStartIdx == INDEX_NONE)
	{
		ThinkStartIdx = OutputMsg.Find(ThinkStartStr, ESearchCase::CaseSensitive, ESearchDir::FromStart, ThinkScanOffset);
		if (ThinkStartIdx != INDEX_NONE)
		{
			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "ThinkStartIdx=%d"), ThinkStartIdx);
			ThinkScanOffset = ThinkStartIdx + ThinkStartStr.Len();
			BatchPos = ThinkScanOffset;
			ThinkFlag = true;
			TagFound = true;
		}
	}

	if (ThinkEndIdx == INDEX_NONE)
	{
		ThinkEndIdx = OutputMsg.Find(ThinkEndStr, ESearchCase::CaseSensitive, ESearchDir::FromStart, ThinkScanOffset);
		if (ThinkEndIdx != INDEX_NONE)
		{
			UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "ThinkEndIdx=%d"), ThinkEndIdx);
			ThinkScanOffset = ThinkEndIdx + ThinkEndStr.Len();
			BatchPos = ThinkScanOffset;
			ThinkFlag = false;
			TagFound = true;

			if (ThinkStartIdx != INDEX_NONE)
			{
				int p0 = ThinkStartIdx + ThinkStartStr.Len();
				int n = ThinkEndIdx - p0;
				if (n > 0)
				{
					FString ThinkStr(OutputMsg.Mid(p0, n));
					UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX "<think> \"%s\""), *ThinkStr);
				}
			}
		}
	}

	if (!TagFound)
	{
		ThinkScanOffset = OutputMsg.Len();
		//UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX "ThinkScanOffset=%d"), ThinkScanOffset);
	}
}

void UHMIChat::AccumulateBatch(const FHMIChatInput& Input)
{
	if (MinBatchLength <= 0 || ThinkFlag || !(OnChatBatch.IsBound() || Input.ChatBatchFunc))
		return;

	const int32 MsgLen = OutputMsg.Len();
	const int32 NSplitChars = BatchSplitCharacters.Len();

	while (BatchPos + MinBatchLength < MsgLen)
	{
		FStringView View(&OutputMsg[BatchPos], MsgLen - BatchPos);
		int32 EndPos = INDEX_NONE;

		for (int32 Idx = 0; Idx < NSplitChars; ++Idx)
		{
			const TCHAR P = BatchSplitCharacters[Idx];
			int32 Pos;
			if (View.FindChar(P, Pos))
			{
				if (Pos >= MinBatchLength)
				{
					EndPos = Pos + 1;
					break;
				}
			}
		}

		if (EndPos == INDEX_NONE && View.Len() > SpaceSplitThreshold)
		{
			for (int32 i = SpaceSplitThreshold; i < View.Len(); ++i)
			{
				if (View[i] == TEXT(' '))
				{
					EndPos = i;
					break;
				}
			}
		}

		if (EndPos == INDEX_NONE && View.Len() > MaxBatchLength)
			EndPos = MaxBatchLength;

		if (EndPos == INDEX_NONE)
			break;

		FStringView BatchView = View.Left(EndPos);
		BroadcastChatBatch(Input, FString(BatchView), false);
		BatchPos += EndPos;

		while (BatchPos < MsgLen && FChar::IsWhitespace(OutputMsg[BatchPos]))
			BatchPos++;
	}
}

void UHMIChat::HandleOperationComplete(bool Success, FString&& Error, FHMIChatInput&& Input)
{
	BroadcastChatDelta(Input, FString(), true);

	if (EnableBatching && (OnChatBatch.IsBound() || Input.ChatBatchFunc))
	{
		const int MsgLen = OutputMsg.Len();
		if (BatchPos < MsgLen)
		{
			BroadcastChatBatch(Input, FString(&OutputMsg[BatchPos], MsgLen - BatchPos), true);
		}
		else
		{
			BroadcastChatBatch(Input, FString(), true);
		}
	}

	BroadcastChatResult(Success, MoveTemp(Error), MoveTemp(Input));
}

void UHMIChat::BroadcastChatDelta(const FHMIChatInput& Input, FString&& Delta, bool EndOfText)
{
	if (HMIChatLogDelta)
	{
		UE_LOG(LogHMI, VeryVerbose, TEXT(LOGPREFIX ">>> (delta) \"%s\""), *Delta);
	}

	auto UserTag = Input.UserTag;

	if (Input.ChatDeltaFunc)
	{
		Input.ChatDeltaFunc(UserTag, Delta, EndOfText);
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, UserTag, Delta = MoveTemp(Delta), EndOfText]() mutable
		{
			OnChatDelta.Broadcast(UserTag, Delta, EndOfText);
		});
	}
}

void UHMIChat::BroadcastChatBatch(const FHMIChatInput& Input, FString&& Batch, bool EndOfText)
{
	UE_LOG(LogHMI, Verbose, TEXT(LOGPREFIX ">>> (batch) \"%s\""), *Batch);

	auto UserTag = Input.UserTag;

	if (Input.ChatBatchFunc)
	{
		Input.ChatBatchFunc(UserTag, Batch, EndOfText);
	}
	else
	{
		AsyncTask(ENamedThreads::GameThread, [this, UserTag, Batch = MoveTemp(Batch), EndOfText]() mutable
		{
			OnChatBatch.Broadcast(UserTag, Batch, EndOfText);
		});
	}
}

void UHMIChat::BroadcastChatResult(bool Success, FString&& Error, FHMIChatInput&& Input)
{
	if (Success)
	{
		UE_LOG(LogHMI, Log, TEXT(LOGPREFIX ">>> (full) \"%s\""), *OutputMsg);
	}

	FHMIChatResult Result(GetTimestamp(), Success, MoveTemp(Error), OutputMsg); // OutputMsg is copied

	BroadcastResult(MoveTemp(Input), MoveTemp(Result), OnChatComplete);
}

#undef LOGPREFIX
