#pragma once

#include "Modules/ModuleManager.h"

class FHMIEditorModule : public IModuleInterface
{
	public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
