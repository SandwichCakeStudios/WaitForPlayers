// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "WaitForPlayersSubsystem.h"

#include "WaitForPlayersControllerComponent.generated.h"


/**
 * 
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class WAITFORPLAYERS_API UWaitForPlayersControllerComponent : public UControllerComponent
{
	GENERATED_BODY()
public:
	UWaitForPlayersControllerComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Send a ready request to the server. 
	 *
	 * @return
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "WaitForPlayersControllerComponent")
	void SendReadyRequest();

	
	
protected:
	


};
