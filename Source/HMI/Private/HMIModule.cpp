#include "HMIModule.h"
#include "HMIMinimal.h"
#include "HMIStatics.h"

DEFINE_LOG_CATEGORY(LogHMI);

void FHMIModule::StartupModule()
{
	UE_LOG(LogHMI, Verbose, TEXT("FHMIModule::StartupModule"));

	UE_LOG(LogHMI, Verbose, TEXT("PluginDir: %s"), *UHMIStatics::GetPluginDir());
	UE_LOG(LogHMI, Verbose, TEXT("BinDir: %s"), *UHMIStatics::GetPluginBinDir());
	UE_LOG(LogHMI, Verbose, TEXT("ThirdPartyBinDir: %s"), *UHMIStatics::GetPluginThirdPartyBinDir());
}

void FHMIModule::ShutdownModule()
{
	UE_LOG(LogHMI, Verbose, TEXT("FHMIModule::ShutdownModule"));
}

IMPLEMENT_MODULE(FHMIModule, HMI)
