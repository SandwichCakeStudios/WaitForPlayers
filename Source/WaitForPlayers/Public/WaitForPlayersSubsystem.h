// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/OnlineReplStructs.h"
#include "WaitForPlayersSubsystem.generated.h"


class UWaitForPlayersGameStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPlayersReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimeoutReached);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnServerStartWaiting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerReady, const FUniqueNetIdRepl, PlayerNetId);

UCLASS(BlueprintType)
class WAITFORPLAYERS_API UWaitForPlayersSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UWaitForPlayersSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void BroadcastAllReady();
	void BroadcastTimeoutReached();
	void BroadcastOnePlayerReady(const FUniqueNetIdRepl& PlayerNetId);
	void BroadcastServerStart();
	

	/**
	 * The server will start listening and wait until all or the maximum players have sent a ready request.
	 * If the timeout is reached, the server will stop listening and execute the OnTimeoutReached Delegate.
	 * You can call the functions GetPlayersReady and GetPlayersNotReady to find out more information.
	 * You can call the function StopWaitingForPlayers to manualy stop.
	 *
	 * @param TimeoutInterval:  The time until server will stop listening. A value of 0 is treated as infinity.
	 * @param MaxPlayersToWaitFor:  The maximum players that have to send ready request. A Value of 0 is treated as all players in the game need to send ready request.
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void StartWaitingForPlayers(float TimeoutInterval = 0, int32 MaxPlayersToWaitFor = 0);

	/**
	 * The server will stop listening.
	 * You can call the functions GetPlayersReady and GetPlayersNotReady to find out more information.
	 *
	 * @param CallDelegates:  If true, Will call the OnAllPlayersReady Delegate for server and all players.
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void StopWaitingForPlayers(bool bCallDelegates);

	/**
	 * If the server should register clients ready requests. This is persistent until you change it.
	 *
	 * @param bAllowClientRequests:  If false, only server can register players as ready.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void AllowClientSendRequests(bool bAllowClientRequests = true);



	/**
	 * Set the timeout interval. Helper incase you want to give more time for players.
	 *
	 * @param Timeout:  Will reset the timer to the new value.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void SetTimeout(float TimeoutInterval = 0);

	/**
	 * Set maximum players needed to send ready requests.
	 *
	 * @param MaxPlayersToWaitFor:  Will update the maximum players needed.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void SetMaxPlayers(int32 MaxPlayersToWaitFor = 0);

	/**
	 * Returns the players who sent ready request.
	 * @return A TArray with all FUniqueNetIdRepl who already sent a ready request. FUniqueNetIdRepl can be used with GetPlayerStatefromUniqueNetId.
	 */
	UFUNCTION(BlueprintCallable, Category = "WaitForPlayersSubsystem")
	const TArray<FUniqueNetIdRepl>  GetPlayersReady();

	/**
	 * Returns the players who hasn´t sent ready request.
	 * @return A TArray with all FUniqueNetIdRepl who hasn´t sent a ready request. FUniqueNetIdRepl can be used with GetPlayerStatefromUniqueNetId.
	 */
	UFUNCTION(BlueprintCallable, Category = "WaitForPlayersSubsystem")
	const TArray<FUniqueNetIdRepl> GetPlayersNotReady();

	/**
	 * Add a UWaitForPlayersControllerComponent at runtime. Only if it does not exist.
	 *
	 * @param PlayerController:  The PlayerController.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void AddControllerToPlayer(APlayerController* PlayerController);

	/**
	 * Removes a UWaitForPlayersControllerComponent at runtime. Only if it does exist.
	 *
	 * @param PlayerController:  The PlayerController.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersSubsystem")
	void RemoveControllerToPlayer(APlayerController* PlayerController);

public:

	UPROPERTY(BlueprintAssignable, Category = "WaitForPlayersSubsystem")
	FOnAllPlayersReady OnAllPlayersReady;

	UPROPERTY(BlueprintAssignable, Category = "WaitForPlayersSubsystem")
	FOnTimeoutReached OnTimeoutReached;
	
	UPROPERTY(BlueprintAssignable, Category = "WaitForPlayersSubsystem")
	FOnServerStartWaiting OnServerStartWaiting;
	
	UPROPERTY(BlueprintAssignable, Category = "WaitForPlayersSubsystem")
	FOnPlayerReady OnPlayerReady;

private:

	UWaitForPlayersGameStateComponent* GetGameStateComponent();
	
};
