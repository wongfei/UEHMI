#include "HMIStatics.h"
#include "Remap/HMIParameterRemapAsset.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"

#if PLATFORM_WINDOWS
	constexpr auto PlatformName = TEXT("Win64");
	constexpr auto PlatformExt = TEXT(".dll");
#elif PLATFORM_LINUX
	constexpr auto PlatformName = TEXT("Linux");
	constexpr auto PlatformExt = TEXT(".so");
#else
	#error "Unsupported platform!"
#endif

static std::atomic<int64> HMIOperationId(1); // TODO: maybe move to subsystem?

int64 UHMIStatics::GenerateOperationId()
{
	return HMIOperationId.fetch_add(1);
}

double UHMIStatics::GetTimestamp(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetRealTimeSeconds() : 0.0;
}

int UHMIStatics::GetNumberOfCores()
{
	return FPlatformMisc::NumberOfCores();
}

FString UHMIStatics::GetPluginDir()
{
	auto Plugin = IPluginManager::Get().FindPlugin(TEXT("HMI"));
	check(Plugin);
	return FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir());
};

FString UHMIStatics::GetPluginBinDir()
{
	return FPaths::Combine(UHMIStatics::GetPluginDir(), TEXT("Binaries"), PlatformName);
};

FString UHMIStatics::GetPluginThirdPartyBinDir()
{
	return FPaths::Combine(UHMIStatics::GetPluginDir(), TEXT("Binaries"), TEXT("ThirdParty"), PlatformName);
}

FString UHMIStatics::GetPluginDllFilepath(const FString& DllNameWithoutExt)
{
	FString Dllpath = FPaths::Combine(UHMIStatics::GetPluginThirdPartyBinDir(), DllNameWithoutExt + PlatformExt);
	if (FPaths::FileExists(Dllpath))
		return Dllpath;

	return FPaths::Combine(UHMIStatics::GetPluginBinDir(), DllNameWithoutExt + PlatformExt);
}

FString UHMIStatics::GetHMIDataDir() // TODO: maybe move to subsystem?
{
	FString EnvOverride = FPlatformMisc::GetEnvironmentVariable(TEXT("HMI_DATA_DIR"));
	if (!EnvOverride.IsEmpty())
	{
		FString Dirpath = FPaths::ConvertRelativePathToFull(EnvOverride);
		if (FPaths::DirectoryExists(Dirpath))
			return Dirpath;
	}

	return FPaths::Combine(UHMIStatics::GetPluginDir(), TEXT("HMI_Data"));
}

FString UHMIStatics::LocateDataFile(const FString& Filename)
{
	if (FPaths::FileExists(Filename))
		return Filename;

	return FPaths::Combine(GetHMIDataDir(), Filename);
}

FString UHMIStatics::LocateDataSubdir(const FString& Dirname)
{
	if (FPaths::DirectoryExists(Dirname))
		return Dirname;

	return FPaths::Combine(GetHMIDataDir(), Dirname);
}

FString UHMIStatics::GetSecret(const FString& Name) // security 9000 lvl
{
	FString EnvOverride = FPlatformMisc::GetEnvironmentVariable(*Name);
	if (!EnvOverride.IsEmpty())
	{
		return EnvOverride;
	}

	FString Filepath = LocateDataFile(Name);
	return ReadStringFromFile(Filepath);
}

TArray<FString> UHMIStatics::FindFiles(const FString& Dirpath, const FString& Pattern)
{
	FString NormalizedPath = FPaths::ConvertRelativePathToFull(Dirpath);
	FString SearchPattern = NormalizedPath / Pattern;

	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(FileNames, *SearchPattern, true, false);

	return FileNames;
}

FString UHMIStatics::ReadStringFromFile(const FString& Filepath)
{
	FString Result;
	if (!FFileHelper::LoadFileToString(Result, *Filepath))
	{
		UE_LOG(LogHMI, Error, TEXT("ReadStringFromFile failed: %s"), *Filepath);
	}

	return Result;
}

TArray<FString> UHMIStatics::ReadLinesFromFile(const FString& Filepath)
{
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *Filepath))
	{
		UE_LOG(LogHMI, Error, TEXT("ReadLinesFromFile failed: %s"), *Filepath);
	}

	return Lines;
}

bool UHMIStatics::WriteStringToFile(const FString& Filepath, const FString& Content, bool Append)
{
	const uint32 Flags = Append ? FILEWRITE_Append : FILEWRITE_None;
	return FFileHelper::SaveStringToFile(Content, *Filepath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), Flags);
}

void UHMIStatics::RemoveSpecialCharacters(FString& Input)
{
	const int Len = Input.Len();
	FString Tmp;
	Tmp.Reserve(Len);

	for (int id = 0; id < Len; ++id)
	{
		TCHAR c = Input[id];

		if (c == TEXT('\r'))
			continue;

		if (c == TEXT('\n'))
			c = TEXT(' ');

		Tmp.AppendChar(c);
	}

	Input = MoveTemp(Tmp);
}

void UHMIStatics::NormalizeText(FString& Input)
{
	UHMIStatics::RemoveSpecialCharacters(Input);
	Input.TrimStartAndEndInline();
}

FString UHMIStatics::RegexReplace(const FString& Input, const FString& Pattern, const FString& Replacement) // TODO: not the brightest one
{
	FString Result = Input;

	FRegexPattern RegexPattern(Pattern);
	FRegexMatcher Matcher(RegexPattern, Input);

	struct FMatch { int32 Begin, End; };
	TArray<FMatch> Matches;

	while (Matcher.FindNext())
	{
		Matches.Add({Matcher.GetMatchBeginning(), Matcher.GetMatchEnding()});
	}

	Matches.Sort([](const FMatch& A, const FMatch& B)
	{
		return A.Begin < B.Begin;
	});

	TArray<FMatch> NonOverlapping;
	int32 LastEnd = -1;

	for (const FMatch& M : Matches)
	{
		if (M.Begin >= LastEnd)
		{
			NonOverlapping.Add(M);
			LastEnd = M.End;
		}
	}

	if (NonOverlapping.Num() > 0)
	{
		for (int32 i = NonOverlapping.Num() - 1; i >= 0; --i)
		{
			const FMatch& M = NonOverlapping[i];
			if (M.End > M.Begin)
			{
				Result = Result.Left(M.Begin) + Replacement + Result.RightChop(M.End);
			}
		}
	}

	return Result;
}
