// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.


#include "WaitForPlayersControllerComponent.h"
#include "Net/UnrealNetwork.h"
#include "WaitForPlayersGameStateComponent.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "WaitForPlayersSubsystem.h"

UWaitForPlayersControllerComponent::UWaitForPlayersControllerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	SetIsReplicatedByDefault(true);
}

void UWaitForPlayersControllerComponent::SendReadyRequest_Implementation()
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersControllerComponent: GetWorld() returned null.")); return; }

	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState)
	{
		UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GameState->FindComponentByClass<UWaitForPlayersGameStateComponent>();
		if (WaitForPlayersGameStateComponent)
		{
			if (WaitForPlayersGameStateComponent->bAllowClientRequests)
			{
				AController* Controller = Cast<AController>(GetOwner());
				if (Controller)
				{
					APlayerState* PlayerState = Controller->PlayerState;
					
					WaitForPlayersGameStateComponent->ServerRegisterReadyRequest(PlayerState->GetUniqueId());
					

				}
			}
		}
	}
}

