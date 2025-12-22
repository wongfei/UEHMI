#include "HMIEditorModule.h"
#include "HMIParameterRemapInputCustomization.h"
#include "HMIParameterRemapOutputCustomization.h"

void FHMIEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"HMIParameterRemapInput",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FHMIParameterRemapInputCustomization::MakeInstance)
	);
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"HMIParameterRemapOutput",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FHMIParameterRemapOutputCustomization::MakeInstance)
	);
}

void FHMIEditorModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FHMIEditorModule, HMIEditor)
