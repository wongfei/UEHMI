#pragma once

#include "HMIMinimal.h"
#include "HMIIndexMapping.generated.h"

UCLASS(BlueprintType, ClassGroup="HMI|Mapping")
class HMI_API UHMIIndexMapping : public UObject
{
	GENERATED_BODY()
	
	// prefix all to prevent collisions with UE
	TMap<int32, FName> HMI_IndexToName;
	TMap<FName, int32> HMI_NameToIndex;

	public:

	void HMI_AddMapping(int32 Id, const FName& Name)
	{
		HMI_IndexToName.Add(Id, Name);
		HMI_NameToIndex.Add(Name, Id);
	}

	UFUNCTION(BlueprintPure, Category="HMI|Mapping")
	FName HMI_GetName(int32 Index) const { return HMI_IndexToName.FindRef(Index); }

	UFUNCTION(BlueprintPure, Category="HMI|Mapping")
	int32 HMI_GetIndex(const FName& Name) const { return HMI_NameToIndex.FindRef(Name); }
};

inline FName HMI_GetName(UHMIIndexMapping* Mapping, int32 Index) { return Mapping ? Mapping->HMI_GetName(Index) : NAME_None; }
inline int32 HMI_GetIndex(UHMIIndexMapping* Mapping, const FName& Name) { return Mapping ? Mapping->HMI_GetIndex(Name) : 0; }

template<typename T>
T HMI_GetValue(const TArray<T>& Array, int32 Index) { return Index >= 0 && Index < Array.Num() ? Array[Index] : T(0); }

template<typename T>
T HMI_GetValue(UHMIIndexMapping* Mapping, const TArray<T>& Array, const FName& Name) { return HMI_GetValue(Array, HMI_GetIndex(Mapping, Name)); }
