#pragma once

#include "Modules/ModuleManager.h"

class FHMIModule : public IModuleInterface
{
	public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
