#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "HMINode_Download.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHMIOnDownloadComplete);

UCLASS()
class HMI_API UHMINode_Download : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	public:

	UPROPERTY(BlueprintAssignable)
	FHMIOnDownloadComplete Complete;

	protected:

	UFUNCTION(BlueprintCallable, Category="HMI|Async", meta=(DisplayName="Download", BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="Urls"))
	static UHMINode_Download* Download_Async(UObject* WorldContextObject, TArray<FString> Urls);

	virtual void Activate() override;

	UPROPERTY(Transient)
	UObject* WorldContext;

	UPROPERTY(Transient)
	TArray<FString> Urls;
};
