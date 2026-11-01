// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "ReadyCheckTypes.h"
#include "ReadyCheckSubsystem.generated.h"

class AController;
class AGameModeBase;
class APlayerController;
class APlayerState;
class UReadyCheckGameStateComponent;
class UReadyCheckPlayerComponent;
class UReadyCheckPolicy;
struct FReadyCheckEntry;
struct FReadyCheckServerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyCheckPhaseChanged, const FReadyCheckSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyCheckProgress, const FReadyCheckSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnReadyCheckPlayerChanged, FGameplayTag, CheckTag, APlayerState*, Player, bool, bReady);

DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnReadyCheckPhaseChangedNative, const FReadyCheckSnapshot&);
DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnReadyCheckProgressNative, const FReadyCheckSnapshot&);
DECLARE_TS_MULTICAST_DELEGATE_ThreeParams(FOnReadyCheckPlayerChangedNative, FGameplayTag, APlayerState*, bool);

/**
 * The single entry point for ready checks. Each check is identified by a Gameplay Tag.
 * Only the server changes state. Every mutating call returns an EReadyCheckResult.
 */
UCLASS()
class WAITFORPLAYERS_API UReadyCheckSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	/** Starts a check with the rules in Policy. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Ready Check")
	EReadyCheckResult StartReadyCheck(FGameplayTag CheckTag, const UReadyCheckPolicy* Policy);

	/** Ends a running check as Cancelled. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Ready Check")
	EReadyCheckResult CancelReadyCheck(FGameplayTag CheckTag);

	/** Ends a running check as Completed, with bForced set. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Ready Check")
	EReadyCheckResult ForceCompleteReadyCheck(FGameplayTag CheckTag);

	/** Sets a player's ready state. Server only. Ignores the policy's rules for player requests: client requests, unready and cooldown. */
	UFUNCTION(BlueprintCallable, Category = "Ready Check")
	EReadyCheckResult SetPlayerReady(APlayerState* Player, FGameplayTag CheckTag, bool bReady);

	/** Adds time to a check in Waiting. Server only. Needs the policy's TimeoutExtension group. */
	UFUNCTION(BlueprintCallable, Category = "Ready Check")
	EReadyCheckResult ExtendTimeout(FGameplayTag CheckTag, float Seconds);

	/** Asks the server to change the local player's ready state. Use it on the owning client or the listen-server host. */
	UFUNCTION(BlueprintCallable, Category = "Ready Check")
	EReadyCheckResult RequestLocalPlayerReady(const APlayerController* LocalController, FGameplayTag CheckTag, bool bReady);

	/** Copies a check's state into OutSnapshot. Returns false if there is no such check. */
	UFUNCTION(BlueprintPure, Category = "Ready Check")
	bool GetSnapshot(FGameplayTag CheckTag, FReadyCheckSnapshot& OutSnapshot) const;

	/** Returns true if the player is ready in this check. */
	UFUNCTION(BlueprintPure, Category = "Ready Check")
	bool IsPlayerReady(const APlayerState* Player, FGameplayTag CheckTag) const;

	/** Gets the players in this check whose ready state matches bReady. */
	UFUNCTION(BlueprintPure, Category = "Ready Check")
	void GetPlayers(FGameplayTag CheckTag, bool bReady, TArray<APlayerState*>& OutPlayers) const;

	/** Returns the seconds left in the current phase, or 0 if it has no deadline. */
	UFUNCTION(BlueprintPure, Category = "Ready Check")
	float GetRemainingTime(FGameplayTag CheckTag) const;

	/** Gets the tags of all checks that exist in this world. */
	UFUNCTION(BlueprintPure, Category = "Ready Check")
	void GetActiveChecks(TArray<FGameplayTag>& OutCheckTags) const;

	/** Fires when a check enters a new phase. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FOnReadyCheckPhaseChanged OnPhaseChanged;

	/** Fires when a check's counts or deadline change. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FOnReadyCheckProgress OnProgressChanged;

	/** Fires when a player's ready state changes. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FOnReadyCheckPlayerChanged OnPlayerReadyChanged;

	/** Native version of OnPhaseChanged. */
	FOnReadyCheckPhaseChangedNative OnPhaseChangedNative;

	/** Native version of OnProgressChanged. */
	FOnReadyCheckProgressNative OnProgressChangedNative;

	/** Native version of OnPlayerReadyChanged. */
	FOnReadyCheckPlayerChangedNative OnPlayerReadyChangedNative;

protected:

	//~ Begin UWorldSubsystem interface
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~ End UWorldSubsystem interface

private:

	friend class UReadyCheckGameStateComponent;
	friend class UReadyCheckPlayerComponent;

	bool HasAuthority() const;
	double GetServerTime() const;
	UReadyCheckGameStateComponent* GetStore() const;

	// Server: requests
	EReadyCheckResult HandleReadyRequest(APlayerState* Player, FGameplayTag CheckTag, bool bReady);
	EReadyCheckResult ApplyPlayerReady(APlayerState* Player, FGameplayTag CheckTag, bool bReady);

	// Server: roster
	void HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void HandleLogout(AGameModeBase* GameMode, AController* Exiting);
	void HandlePlayerJoined(APlayerState& Player);
	void HandlePlayerLeft(APlayerState& Player);
	bool UpdateMembership(APlayerState& Player, FReadyCheckSnapshot& Snapshot, FReadyCheckServerState& State);
	void RemoveMember(UReadyCheckPlayerComponent& PlayerComponent, FReadyCheckSnapshot& Snapshot, FReadyCheckServerState& State);

	// Server: reconnect
	void RememberAwayPlayer(const APlayerState& Player, bool bReady, FGameplayTag CheckTag, FReadyCheckServerState& State);
	bool RestoreAwayPlayer(APlayerState& Player, FReadyCheckEntry& Entry, FReadyCheckServerState& State);
	void ForgetAwayPlayers(FReadyCheckServerState& State);
	void ArmAwayTimer(FGameplayTag CheckTag, FReadyCheckServerState& State);
	void HandleAwayExpiry(FGameplayTag CheckTag);

	// Server: phase changes
	void ReevaluateCheck(FGameplayTag CheckTag, double Now);
	void CommitSnapshot(UReadyCheckGameStateComponent& Store, FReadyCheckEntry& Entry, FReadyCheckServerState& State, const FReadyCheckSnapshot& NewSnapshot);
	void ArmPhaseTimer(const FReadyCheckSnapshot& Snapshot, FReadyCheckServerState& State);
	void HandlePhaseDeadline(FGameplayTag CheckTag);

	// Server and clients: events
	void NotifySnapshotChanged(FReadyCheckEntry& Entry);
	void NotifyPlayerReadyChanged(FGameplayTag CheckTag, APlayerState* Player, bool bReady);

	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;
};
