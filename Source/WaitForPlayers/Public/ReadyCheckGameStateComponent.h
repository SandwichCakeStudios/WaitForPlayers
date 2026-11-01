// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/OnlineReplStructs.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ReadyCheckTypes.h"
#include "ReadyCheckGameStateComponent.generated.h"

class UReadyCheckGameStateComponent;
class UReadyCheckPolicy;
struct FReadyCheckEntryArray;

/** One check in the replicated store. Entries stay after a check ends, so late joiners see the outcome. */
USTRUCT()
struct FReadyCheckEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** The state every machine converges on. */
	UPROPERTY()
	FReadyCheckSnapshot Snapshot;

	/** The state this machine last raised events for. A new entry starts as Inactive. */
	UPROPERTY(NotReplicated)
	FReadyCheckSnapshot LastNotified;

	void PostReplicatedAdd(const FReadyCheckEntryArray& InArraySerializer);
	void PostReplicatedChange(const FReadyCheckEntryArray& InArraySerializer);
};

/** The fast array of checks. A change to one check sends only that check. */
USTRUCT()
struct FReadyCheckEntryArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FReadyCheckEntry> Items;

	/** Set once by the owning component. Not a UPROPERTY, so it is never copied from the CDO. */
	UReadyCheckGameStateComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FReadyCheckEntry, FReadyCheckEntryArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FReadyCheckEntryArray> : public TStructOpsTypeTraitsBase2<FReadyCheckEntryArray>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

/** What the phase machine keeps between evaluations of a check. Never replicated. */
struct FReadyCheckPhaseMemory
{
	/** The check has had MinPlayers at some point, so dropping below it now cancels the check. */
	bool bMinPlayersReached = false;

	/** The seconds Waiting had left when Countdown began. 0 means Waiting had no deadline. */
	double WaitingTimeLeft = 0.0;

	/** How many times the timeout was extended. */
	int32 NumExtensions = 0;

	/** The seconds all extensions added together. */
	double TotalExtraTime = 0.0;
};

/** A player who left a check and may come back. */
struct FReadyCheckAwayPlayer
{
	/** Whether the player was ready when they left. */
	bool bReady = false;

	/** When the player left, in server world time. */
	double LeaveServerTime = 0.0;
};

/** The server-only side of a check. */
USTRUCT()
struct FReadyCheckServerState
{
	GENERATED_BODY()

	/** The rules this check started with. */
	UPROPERTY()
	TObjectPtr<const UReadyCheckPolicy> Policy;

	/** How many players are in this check. Updated per join and leave, never counted by a scan. */
	int32 NumEligible = 0;

	/** The phase machine's own state for this check. */
	FReadyCheckPhaseMemory PhaseMemory;

	/** Fires at the current phase's deadline. */
	FTimerHandle PhaseTimer;

	/** Players who left and may come back. Stays empty unless the policy's Reconnect group is on. */
	TMap<FUniqueNetIdRepl, FReadyCheckAwayPlayer> AwayPlayers;

	/** Fires when the next held slot expires. Only used with bHoldSlotWhileAway and a GracePeriod. */
	FTimerHandle AwayTimer;
};

/**
 * The authoritative store of ready checks. It replicates to every client through the GameState.
 * The subsystem adds it at runtime on the first StartReadyCheck, unless it is already on the GameState.
 */
UCLASS(MinimalAPI, ClassGroup = "Ready Check", meta = (BlueprintSpawnableComponent))
class UReadyCheckGameStateComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:

	UReadyCheckGameStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const TArray<FReadyCheckEntry>& GetEntries() const { return Entries.Items; }

	FReadyCheckEntry* FindEntry(FGameplayTag CheckTag);
	const FReadyCheckEntry* FindEntry(FGameplayTag CheckTag) const;

	/** Server only. Returns the entry for this tag, adding one if needed. */
	FReadyCheckEntry& FindOrAddEntry(FGameplayTag CheckTag);

	/** Server only. Queues the entry for replication. */
	void MarkEntryDirty(FReadyCheckEntry& Entry);

	/** Server only. Returns the side state of a started check, or null. */
	FReadyCheckServerState* FindServerState(FGameplayTag CheckTag);

	/** Server only. Returns the side state for this tag, adding one if needed. */
	FReadyCheckServerState& FindOrAddServerState(FGameplayTag CheckTag);

	/** Client only. Raises the events for an entry that just replicated. */
	void HandleEntryReplicated(FReadyCheckEntry& Entry);

private:

	UPROPERTY(Replicated)
	FReadyCheckEntryArray Entries;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FReadyCheckServerState> ServerStates;
};
