#pragma once

#include "HMIMinimal.h"
#include "OpenAITypes.generated.h"

USTRUCT(BlueprintType, Category="OpenAI")
struct HMI_API FOpenAIError
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString code;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString message;
};

USTRUCT(BlueprintType, Category="OpenAI")
struct HMI_API FOpenAIChatMsg
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString role;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString content;
};

USTRUCT(BlueprintType, Category="OpenAI")
struct HMI_API FOpenAIChatChoice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	int index = 0;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FOpenAIChatMsg delta;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FOpenAIChatMsg message;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString finish_reason;
};

USTRUCT(BlueprintType, Category="OpenAI")
struct HMI_API FOpenAIChatCompletionRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString model;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	TArray<FOpenAIChatMsg> messages;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	bool stream = false;
};

USTRUCT(BlueprintType, Category="OpenAI")
struct HMI_API FOpenAIChatCompletionResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FOpenAIError error;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString id;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString object;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	int created = 0; // unix timestamp in seconds

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString model;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	FString system_fingerprint;

	UPROPERTY(BlueprintReadWrite, Category="OpenAI")
	TArray<FOpenAIChatChoice> choices;
};
