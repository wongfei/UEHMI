#pragma once

#include "HMIMinimal.h"
#include "Misc/Paths.h"

#include "HMIThirdPartyBegin.h"
#include "SherpaLoader.h"
#include "HMIThirdPartyEnd.h"

#include <string>

#define SHERPA_CALL(name, args) SherpaAPI->name##_fp args

#define SHERPA_REQUIRED_PATH(Field, ParamName, NodeType) do {\
	Field = Helper.GetPath(ParamName, NodeType);\
	if (!Field) {\
		FString Path = GetProcessorString(ParamName);\
		ProcError(FString::Printf(TEXT("Path not found: %s -> %s"), TEXT(ParamName), *Path));\
		return false;\
	}\
	} while (false)

enum { NullIfEmpty = 1 };
enum { Node_File = 0, Node_Dir = 1 };

class FSherpaHelper
{
	public:

	UHMIProcessor* Proc;
	FString& ModelPath;
	TMap<FAnsiString, FAnsiString>& SherpaStrings;
	FString Quant; // int8 fp16

	FSherpaHelper(UHMIProcessor* InProc, FString& InModelPath, TMap<FAnsiString, FAnsiString>& InSherpaStrings, const FString& InQuant) : 
		Proc(InProc),
		ModelPath(InModelPath),
		SherpaStrings(InSherpaStrings),
		Quant(InQuant)
	{}

	const char* GetStr(const char* ParamName, const FString& Value, int Flag = 0)
	{
		if (Value.IsEmpty() && Flag == NullIfEmpty)
			return (const char*)nullptr;

		const char* ValuePtr = *SherpaStrings.Add({ ParamName, TCHAR_TO_UTF8(*Value) });
		UE_LOG(LogHMI, VeryVerbose, TEXT("%s = %s"), UTF8_TO_TCHAR(ParamName), UTF8_TO_TCHAR(ValuePtr));

		return ValuePtr;
	};

	const char* GetStr(const char* ParamName, int Flag = 0)
	{
		return GetStr(ParamName, Proc->GetProcessorString(ParamName), Flag);
	};

	bool GetBool(const char* ParamName) const
	{
		const bool Value = Proc->GetProcessorBool(ParamName);
		UE_LOG(LogHMI, VeryVerbose, TEXT("%s = %d"), UTF8_TO_TCHAR(ParamName), Value ? 1 : 0);
		return Value;
	};

	int GetInt(const char* ParamName) const
	{
		const int Value = Proc->GetProcessorInt(ParamName);
		UE_LOG(LogHMI, VeryVerbose, TEXT("%s = %d"), UTF8_TO_TCHAR(ParamName), Value);
		return Value;
	};

	float GetFloat(const char* ParamName) const
	{
		const float Value = Proc->GetProcessorFloat(ParamName);
		UE_LOG(LogHMI, VeryVerbose, TEXT("%s = %f"), UTF8_TO_TCHAR(ParamName), Value);
		return Value;
	};

	const char* GetPath(const char* ParamName, int NodeType)
	{
		FString Name = Proc->GetProcessorString(ParamName);

		// pattern
		int32 Idx;
		if (NodeType == Node_File && Name.FindChar(TEXT('*'), Idx))
		{
			auto Files = UHMIStatics::FindFiles(ModelPath, Name);
			if (!Files.IsEmpty())
			{
				if (Name.Contains(TEXT(".onnx")))
				{
					for (const auto& File : Files)
					{
						if ((Quant.IsEmpty() && !File.Contains(TEXT("int8")) && !File.Contains(TEXT("fp16")))
							|| (!Quant.IsEmpty() && File.Contains(Quant)))
						{
							return GetStr(ParamName, FPaths::Combine(ModelPath, File));
						}
					}
				}
				return GetStr(ParamName, FPaths::Combine(ModelPath, Files[0]));
			}
		}

		// exact match
		FString Path = FPaths::Combine(ModelPath, Name);
		if (!Name.IsEmpty() && (NodeType == Node_File ? FPaths::FileExists(Path) : FPaths::DirectoryExists(Path)))
		{
			return GetStr(ParamName, Path);
		}

		return (const char*)nullptr;
	};

	const char* GetLexicon(const char* ParamName)
	{
		FString Lexicon = Proc->GetProcessorString(ParamName); // "us-en,zh"
		if (!Lexicon.IsEmpty())
		{
			TArray<FString> Parts;
			Lexicon.ParseIntoArray(Parts, TEXT(","));

			TArray<FString> LexPaths;
			for (const auto& Part : Parts)
			{
				auto Files = UHMIStatics::FindFiles(ModelPath, FString::Printf(TEXT("lex*%s*.txt"), *Part));
				if (!Files.IsEmpty())
				{
					LexPaths.Add(FPaths::Combine(ModelPath, Files[0]));
				}
				else
				{
					UE_LOG(LogHMI, Warning, TEXT("Lexicon not found: %s"), *Part);
				}
			}

			if (!LexPaths.IsEmpty())
			{
				Lexicon = FString::Join(LexPaths, TEXT(","));
				return GetStr(ParamName, Lexicon);
			}
		}

		return (const char*)nullptr;
	};
};
