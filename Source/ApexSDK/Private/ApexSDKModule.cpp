// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

#include "Modules/ModuleManager.h"

class FApexSDKModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// The authorization-code login needs WebSockets loaded before the first socket is made.
		FModuleManager::Get().LoadModule(TEXT("WebSockets"));
	}
};

IMPLEMENT_MODULE(FApexSDKModule, ApexSDK)
