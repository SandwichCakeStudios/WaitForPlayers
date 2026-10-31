// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "Engine/TimerHandle.h"
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

	/** Fires at the current phase's deadline. */
	FTimerHandle PhaseTimer;
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
