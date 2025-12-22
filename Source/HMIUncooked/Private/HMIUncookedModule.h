#pragma once

#include "Modules/ModuleManager.h"

class FHMIUncookedModule : public IModuleInterface
{
	public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
