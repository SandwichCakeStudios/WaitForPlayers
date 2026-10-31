// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PlayerStateComponent.h"
#include "GameplayTagContainer.h"
#include "ReadyCheckPlayerComponent.generated.h"

/** One player's state in one check. Having an entry means the player is part of that check. */
USTRUCT()
struct FReadyCheckPlayerEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag CheckTag;

	UPROPERTY()
	bool bReady = false;
};

/**
 * One player's ready state, usually for 1 to 3 checks. A change replicates only this small array.
 * The PlayerState belongs to the player's connection, so requests need no controller component.
 * The subsystem adds it at runtime, unless it is already on the PlayerState.
 */
UCLASS(MinimalAPI, ClassGroup = "Ready Check", meta = (BlueprintSpawnableComponent))
class UReadyCheckPlayerComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:

	UReadyCheckPlayerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const FReadyCheckPlayerEntry* FindEntry(FGameplayTag CheckTag) const;

	/** Server only. Makes this player part of a check, not ready. */
	void AddEntry(FGameplayTag CheckTag);

	/** Server only. Takes this player out of a check. */
	void RemoveEntry(FGameplayTag CheckTag);

	/** Server only. Sets the ready state of an existing entry. */
	void SetEntryReady(FGameplayTag CheckTag, bool bReady);

	/** Asks the server to change this player's ready state. */
	UFUNCTION(Server, Reliable)
	void ServerRequestReady(FGameplayTag CheckTag, bool bReady);

private:

	UFUNCTION()
	void OnRep_Entries(const TArray<FReadyCheckPlayerEntry>& OldEntries);

	void MarkEntriesDirty();

	UPROPERTY(ReplicatedUsing = OnRep_Entries)
	TArray<FReadyCheckPlayerEntry> Entries;
};
