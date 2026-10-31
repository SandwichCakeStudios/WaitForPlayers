// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

WAITFORPLAYERS_API DECLARE_LOG_CATEGORY_EXTERN(LogWaitForPlayers, Log, All);

class FWaitForPlayersModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
