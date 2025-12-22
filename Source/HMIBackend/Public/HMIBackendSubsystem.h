#pragma once

#include "HMIMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "HMIBackendSubsystem.generated.h"

class UHMIProcessor;

UCLASS(NotBlueprintType, NotBlueprintable)
class HMIBACKEND_API UHMIBackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	public:

	static UHMIBackendSubsystem* GetInstance(UObject* WorldContextObject);

	UHMIBackendSubsystem();
	~UHMIBackendSubsystem();

	void TickSubsystem(float DeltaTime);

	// USubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION()
	void OnProcessorCreated(UHMIProcessor* Processor);

	protected:

	FTSTicker::FDelegateHandle TickHandle;
};
