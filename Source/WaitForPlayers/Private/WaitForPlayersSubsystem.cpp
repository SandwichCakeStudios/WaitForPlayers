// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.


#include "WaitForPlayersSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "WaitForPlayersControllerComponent.h"
#include "WaitForPlayersGameStateComponent.h"


UWaitForPlayersSubsystem::UWaitForPlayersSubsystem()
{
	
}

void UWaitForPlayersSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	
}

void UWaitForPlayersSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UWaitForPlayersSubsystem::BroadcastAllReady()
{
	OnAllPlayersReady.Broadcast();
}
void UWaitForPlayersSubsystem::BroadcastTimeoutReached()
{
	OnTimeoutReached.Broadcast();
}
void UWaitForPlayersSubsystem::BroadcastOnePlayerReady(const FUniqueNetIdRepl& PlayerNetId)
{
	
	OnPlayerReady.Broadcast(PlayerNetId);
}

void UWaitForPlayersSubsystem::BroadcastServerStart()
{
	OnServerStartWaiting.Broadcast();
}

void UWaitForPlayersSubsystem::StartWaitingForPlayers(float Timeout, int32 MaxPlayersToWaitFor)
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersSubsystem: GetWorld() returned null.")); return; }
	AGameStateBase* GameState = GetWorld()->GetGameState();
	
	if (GameState)
	{
		UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GameState->FindComponentByClass<UWaitForPlayersGameStateComponent>();
		
		//If the gamestate component never was added, let´s add it now.
		if (!WaitForPlayersGameStateComponent)
		{
			WaitForPlayersGameStateComponent = NewObject<UWaitForPlayersGameStateComponent>(GameState, TEXT("WaitForPlayersGameStateComponent"));

			WaitForPlayersGameStateComponent->RegisterComponent();
			GameState->AddInstanceComponent(WaitForPlayersGameStateComponent);
			
		}
		
		
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			// Add the controller component incase it does not exist.
			if (PlayerState)
			{
				APlayerController* PlayerController = PlayerState->GetPlayerController();
				AddControllerToPlayer(PlayerController);
				
			}
		}


		WaitForPlayersGameStateComponent->TimeoutInterval = Timeout;
		WaitForPlayersGameStateComponent->MaxPlayersToWaitFor = MaxPlayersToWaitFor;

		WaitForPlayersGameStateComponent->StartServerListener();

	}
}

void UWaitForPlayersSubsystem::StopWaitingForPlayers(bool bCallDelegates)
{
	
	UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GetGameStateComponent();

	
	if (WaitForPlayersGameStateComponent)
	{
		WaitForPlayersGameStateComponent->StopServerListener(bCallDelegates);
	}
	
}

void UWaitForPlayersSubsystem::AllowClientSendRequests(bool bAllowClientRequests)
{
	UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GetGameStateComponent();


	if (WaitForPlayersGameStateComponent)
	{
		WaitForPlayersGameStateComponent->bAllowClientRequests = bAllowClientRequests;
		

	}
}

void UWaitForPlayersSubsystem::SetTimeout(float TimeoutInterval)
{
	UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GetGameStateComponent();


	if (WaitForPlayersGameStateComponent)
	{
		WaitForPlayersGameStateComponent->TimeoutInterval = TimeoutInterval;
		WaitForPlayersGameStateComponent->StopTimer();

		if (TimeoutInterval > 0) 
		{
			WaitForPlayersGameStateComponent->StartTimer();
		}
	
	}
}

void UWaitForPlayersSubsystem::SetMaxPlayers(int32 MaxPlayersToWaitFor)
{
	UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GetGameStateComponent();


	if (WaitForPlayersGameStateComponent)
	{
		WaitForPlayersGameStateComponent->SetMaxPlayers(MaxPlayersToWaitFor);
	}
}

const TArray<FUniqueNetIdRepl> UWaitForPlayersSubsystem::GetPlayersReady()
{
	UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GetGameStateComponent();


	if (WaitForPlayersGameStateComponent)
	{
		return WaitForPlayersGameStateComponent->PlayerReadyList;
	}

	return TArray<FUniqueNetIdRepl>();
}

const TArray<FUniqueNetIdRepl> UWaitForPlayersSubsystem::GetPlayersNotReady()
{

	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersSubsystem: GetWorld() returned null.")); return TArray<FUniqueNetIdRepl>();}

	AGameStateBase* GameState = GetWorld()->GetGameState();

	if (GameState)
	{
		UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GetGameStateComponent();


		if (WaitForPlayersGameStateComponent)
		{
		
			TArray<FUniqueNetIdRepl> PlayersNotReadyList;

			for (APlayerState* PlayerState : GameState->PlayerArray)
			{
		
				if (PlayerState)
				{
					bool bIsPlayerReady= WaitForPlayersGameStateComponent->PlayerReadyList.Contains(PlayerState->GetUniqueId());
					if(!bIsPlayerReady)
					{
						PlayersNotReadyList.Add(PlayerState->GetUniqueId());
					}
				}
			}

			return PlayersNotReadyList;
		}
	}

	return TArray<FUniqueNetIdRepl>();
}




void UWaitForPlayersSubsystem::AddControllerToPlayer(APlayerController* PlayerController)
{
	if (PlayerController)
	{
		UWaitForPlayersControllerComponent* FoundControllerComponent = PlayerController->FindComponentByClass<UWaitForPlayersControllerComponent>();
		if (!FoundControllerComponent)
		{
			UWaitForPlayersControllerComponent* WaitForPlayersControllerComponent = NewObject<UWaitForPlayersControllerComponent>(PlayerController, TEXT("WaitForPlayersControllerComponent"));

			WaitForPlayersControllerComponent->RegisterComponent();
			PlayerController->AddInstanceComponent(WaitForPlayersControllerComponent);
		}
	}
}

void UWaitForPlayersSubsystem::RemoveControllerToPlayer(APlayerController* PlayerController)
{
	UWaitForPlayersControllerComponent* WaitForPlayersControllerComponent = PlayerController->FindComponentByClass<UWaitForPlayersControllerComponent>();
	if (WaitForPlayersControllerComponent)
	{
		WaitForPlayersControllerComponent->UnregisterComponent();
		PlayerController->RemoveInstanceComponent(WaitForPlayersControllerComponent);
		

	}
}

UWaitForPlayersGameStateComponent* UWaitForPlayersSubsystem::GetGameStateComponent()
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersSubsystem: GetWorld() returned null.")); return nullptr; }

	AGameStateBase* GameState = GetWorld()->GetGameState();

	if (GameState)
	{
		UWaitForPlayersGameStateComponent* WaitForPlayersGameStateComponent = GameState->FindComponentByClass<UWaitForPlayersGameStateComponent>();

		
		if (WaitForPlayersGameStateComponent)
		{
			return WaitForPlayersGameStateComponent;
		}
	}

	return nullptr;
}
