// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.


#include "WaitForPlayersGameStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"


UWaitForPlayersGameStateComponent::UWaitForPlayersGameStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	SetIsReplicatedByDefault(true);
	TimeoutInterval = 30.0f;
	MaxPlayersToWaitFor = 0;
	DelayAllReady = 3.0f;
	bAllowClientRequests = true;
	bIsWaitingForPlayers = false;
}





void UWaitForPlayersGameStateComponent::BeginPlay()
{
	Super::BeginPlay();

	
	if (GetOwnerRole() == ROLE_Authority)
	{
		
		// Reset number of players ready
		NumPlayersReady = 0;
	}
}

void UWaitForPlayersGameStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
		World->GetTimerManager().ClearTimer(AllReadyTimerHandle);
		
	}

	Super::EndPlay(EndPlayReason);
}



void UWaitForPlayersGameStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWaitForPlayersGameStateComponent, PlayerReadyList);
}



void UWaitForPlayersGameStateComponent::ServerRegisterReadyRequest(const FUniqueNetIdRepl& PlayerNetId)
{
	
	if (!bAllowClientRequests) { return; }
	if (!HasAuthority()) { return; }
	if (!bIsWaitingForPlayers) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: Server is not listening")); return; }
	
	//No dublicates.
	bool bPlayerAlreadySent = PlayerReadyList.Contains(PlayerNetId);

	if(!bPlayerAlreadySent)
	{
		
		// Set player ready status and update count
		PlayerReadyList.Add(PlayerNetId);
		NumPlayersReady++;

		MulticastOnPlayerReady(PlayerNetId);

		// Check if all players are ready
		CheckAllPlayersReady();
	}
	
}

void UWaitForPlayersGameStateComponent::StartServerListener()
{
	
	bIsWaitingForPlayers = true;

	//Clear players ready
	NumPlayersReady = 0;
	PlayerReadyList.Empty();

	if (TimeoutInterval > 0)
	{
		StartTimer();
	}

	MulticastOnServerStart();
}

void UWaitForPlayersGameStateComponent::StopServerListener(bool bCallDelegates)
{
	
	bIsWaitingForPlayers = false;
	StopTimer();
	
	// Broadcast the event to notify all players that all players are ready
	if(bCallDelegates)
	{
		if (UWorld* World = GetWorld())
		{
			//Add a small delay, in normal multiplayer condition this delay should be enough to ensure all players ready list is uptodate.
			//However a FFastArraySerializer whould be better but you know.. free plugin... :)
			World->GetTimerManager().SetTimer(AllReadyTimerHandle, this, &UWaitForPlayersGameStateComponent::OnAllReadyComplete, DelayAllReady);
		}
	}
	
}

void UWaitForPlayersGameStateComponent::SetMaxPlayers(int32 InMaxPlayersToWaitFor)
{
	MaxPlayersToWaitFor = InMaxPlayersToWaitFor;
	// Check if all players are ready
	CheckAllPlayersReady();
}

void UWaitForPlayersGameStateComponent::StartTimer()
{
	if (!bIsWaitingForPlayers) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: Server is not listening")); return; }

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CoolDownTimerHandle, this, &UWaitForPlayersGameStateComponent::OnTimeoutComplete, TimeoutInterval);
	}
}

void UWaitForPlayersGameStateComponent::StopTimer()
{
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CoolDownTimerHandle);
	}

	

}

void UWaitForPlayersGameStateComponent::OnTimeoutComplete()
{
	StopServerListener(false);
	MulticastOnTimeoutReached();
}

void UWaitForPlayersGameStateComponent::OnAllReadyComplete()
{
	MulticastOnAllPlayersReady();
}


void UWaitForPlayersGameStateComponent::MulticastOnAllPlayersReady_Implementation()
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: GetWorld() returned null.")); return; }
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();

	if (GameInstance)
	{
		UWaitForPlayersSubsystem* WaitForPlayersSubsystem = GameInstance->GetSubsystem<UWaitForPlayersSubsystem>();

		if (WaitForPlayersSubsystem)
		{
			WaitForPlayersSubsystem->BroadcastAllReady();
		}
	}
}



void UWaitForPlayersGameStateComponent::MulticastOnServerStart_Implementation()
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: GetWorld() returned null.")); return; }
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();

	if (GameInstance)
	{
		UWaitForPlayersSubsystem* WaitForPlayersSubsystem = GameInstance->GetSubsystem<UWaitForPlayersSubsystem>();

		if (WaitForPlayersSubsystem)
		{
			WaitForPlayersSubsystem->BroadcastServerStart();
		}
	}
}
void UWaitForPlayersGameStateComponent::MulticastOnTimeoutReached_Implementation()
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: GetWorld() returned null.")); return; }
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();

	if (GameInstance)
	{
		UWaitForPlayersSubsystem* WaitForPlayersSubsystem = GameInstance->GetSubsystem<UWaitForPlayersSubsystem>();

		if (WaitForPlayersSubsystem)
		{
			WaitForPlayersSubsystem->BroadcastTimeoutReached();
		}
	}
}

void UWaitForPlayersGameStateComponent::MulticastOnPlayerReady_Implementation(const FUniqueNetIdRepl PlayerNetId)
{
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: GetWorld() returned null.")); return; }
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	
	if(GameInstance)
	{
		
		UWaitForPlayersSubsystem* WaitForPlayersSubsystem = GameInstance->GetSubsystem<UWaitForPlayersSubsystem>();

		if (WaitForPlayersSubsystem)
		{
			
			WaitForPlayersSubsystem->BroadcastOnePlayerReady(PlayerNetId);
		}
	}
}

void UWaitForPlayersGameStateComponent::CheckAllPlayersReady()
{
	if (!HasAuthority()) { return; }
	if (!bIsWaitingForPlayers) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: Server is not listening")); return; }
	if (!GetWorld()) { UE_LOG(LogTemp, Warning, TEXT("UWaitForPlayersGameStateComponent: GetWorld() returned null.")); return; }

	bool PlayersReady = false;
	
	if (MaxPlayersToWaitFor > 0 && NumPlayersReady >= MaxPlayersToWaitFor)
	{
	
		PlayersReady = true;
	}
	else if (NumPlayersReady >= GetWorld()->GetAuthGameMode()->GetNumPlayers())
	{
		PlayersReady = true;
	}

	if (PlayersReady) 
	{
		
		StopServerListener(true);
		
	}
}