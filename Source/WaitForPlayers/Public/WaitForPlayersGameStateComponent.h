// Copyright 2023 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Net/UnrealNetwork.h"
#include "Components/GameStateComponent.h"
#include "WaitForPlayersSubsystem.h"
#include "WaitForPlayersGameStateComponent.generated.h"


/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class WAITFORPLAYERS_API UWaitForPlayersGameStateComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UWaitForPlayersGameStateComponent(const FObjectInitializer& ObjectInitializer);



	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Register the player with PlayerNetId as ready.
	 * You can use this when the server should decide when a player is ready. If the player should decide when it´s ready, for example clicking on a button in UI,
	 * Use the SendReadyRequest inside WaitForPlayersControllerComponent.
	 *
	 * @param PlayerNetId:  The uniqueNetId(FUniqueNetIdRepl) of the player.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersGameStateComponent")
	void ServerRegisterReadyRequest(const FUniqueNetIdRepl& PlayerNetId);
	
	void StartServerListener();

	void StopServerListener(bool bCallDelegates);

	/**
	 * Set maximum players needed to send ready requests. If server is
	 * already listening, will also recheck if enough players are ready.
	 *
	 * @param MaxPlayersToWaitFor:  Will update the maximum players needed.
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersGameStateComponent")
	void SetMaxPlayers(int32 InMaxPlayersToWaitFor = 0);

	void StartTimer();

	/**
	 * Stop the Timeout timer. 
	 *
	 * @return
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "WaitForPlayersGameStateComponent")
	void StopTimer();

	UFUNCTION()
	void OnTimeoutComplete();

	UFUNCTION()
	void OnAllReadyComplete();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnAllPlayersReady();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnTimeoutReached();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnServerStart();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPlayerReady(const FUniqueNetIdRepl PlayerNetId);

	

	
	const TArray<FUniqueNetIdRepl>& GetPlayersReady() { return PlayerReadyList; }

public:
	/**
	 * The TimeoutInterval, time when server stop listening.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaitForPlayersGameStateComponent")
	float TimeoutInterval;

	/**
	 * Max players to wait for. 
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaitForPlayersGameStateComponent")
	int32 MaxPlayersToWaitFor;

	/**
	 * A delay before broadcasting the AllPlayersReady Event.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WaitForPlayersGameStateComponent")
	float DelayAllReady;

	UPROPERTY()
	bool bIsWaitingForPlayers;

	UPROPERTY()
	bool bAllowClientRequests;
	

	UPROPERTY(Replicated)
	TArray<FUniqueNetIdRepl> PlayerReadyList;

	UPROPERTY()
	int32 NumPlayersReady;

protected:
	


private:

	void CheckAllPlayersReady();

	FTimerHandle CoolDownTimerHandle;
	FTimerHandle AllReadyTimerHandle;
};
