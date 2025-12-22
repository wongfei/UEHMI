#pragma once

#include "Modules/ModuleManager.h"

class FHMIBackendModule : public IModuleInterface
{
	public:

	static inline FHMIBackendModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FHMIBackendModule>("HMIBackend");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	bool HaveOpenCV() const { return bHaveOpenCV; }
	bool HaveOnnx() const { return bHaveOnnx; }
	bool HaveSherpa() const { return bHaveSherpa; }
	bool HavePiper() const { return bHavePiper; }
	bool HaveOVRLipSync() const { return bHaveOVRLipSync; }

	struct FSherpaAPI* GetSherpaAPI() { return SherpaAPI.Get(); }
	struct piper_api* GetPiperAPI() { return PiperAPI.Get(); }

	private:

	void* OpenCVHandle = nullptr;
	void* OnnxHandle = nullptr;
	void* PiperHandle = nullptr;
	TUniquePtr<struct FSherpaAPI> SherpaAPI;
	TUniquePtr<struct piper_api> PiperAPI;
	
	bool bHaveOpenCV = false;
	bool bHaveOnnx = false;
	bool bHaveSherpa = false;
	bool bHavePiper = false;
	bool bHaveOVRLipSync = false;
};
