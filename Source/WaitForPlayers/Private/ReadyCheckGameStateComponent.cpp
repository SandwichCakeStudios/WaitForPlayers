// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckGameStateComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "ReadyCheckPolicy.h"
#include "ReadyCheckSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckGameStateComponent)

void FReadyCheckEntry::PostReplicatedAdd(const FReadyCheckEntryArray& InArraySerializer)
{
	if (InArraySerializer.Owner != nullptr)
	{
		InArraySerializer.Owner->HandleEntryReplicated(*this);
	}
}

void FReadyCheckEntry::PostReplicatedChange(const FReadyCheckEntryArray& InArraySerializer)
{
	if (InArraySerializer.Owner != nullptr)
	{
		InArraySerializer.Owner->HandleEntryReplicated(*this);
	}
}

UReadyCheckGameStateComponent::UReadyCheckGameStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	Entries.Owner = this;
}

void UReadyCheckGameStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UReadyCheckGameStateComponent, Entries, Params);
}

FReadyCheckEntry* UReadyCheckGameStateComponent::FindEntry(FGameplayTag CheckTag)
{
	return Entries.Items.FindByPredicate([CheckTag](const FReadyCheckEntry& Entry)
	{
		return Entry.Snapshot.CheckTag == CheckTag;
	});
}

const FReadyCheckEntry* UReadyCheckGameStateComponent::FindEntry(FGameplayTag CheckTag) const
{
	return Entries.Items.FindByPredicate([CheckTag](const FReadyCheckEntry& Entry)
	{
		return Entry.Snapshot.CheckTag == CheckTag;
	});
}

FReadyCheckEntry& UReadyCheckGameStateComponent::FindOrAddEntry(FGameplayTag CheckTag)
{
	if (FReadyCheckEntry* Entry = FindEntry(CheckTag))
	{
		return *Entry;
	}

	FReadyCheckEntry& Entry = Entries.Items.AddDefaulted_GetRef();
	Entry.Snapshot.CheckTag = CheckTag;
	Entry.LastNotified.CheckTag = CheckTag;
	return Entry;
}

void UReadyCheckGameStateComponent::MarkEntryDirty(FReadyCheckEntry& Entry)
{
	Entries.MarkItemDirty(Entry);
	MARK_PROPERTY_DIRTY_FROM_NAME(UReadyCheckGameStateComponent, Entries, this);

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

FReadyCheckServerState* UReadyCheckGameStateComponent::FindServerState(FGameplayTag CheckTag)
{
	return ServerStates.Find(CheckTag);
}

FReadyCheckServerState& UReadyCheckGameStateComponent::FindOrAddServerState(FGameplayTag CheckTag)
{
	return ServerStates.FindOrAdd(CheckTag);
}

void UReadyCheckGameStateComponent::HandleEntryReplicated(FReadyCheckEntry& Entry)
{
	const UWorld* World = GetWorld();
	if (UReadyCheckSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<UReadyCheckSubsystem>() : nullptr)
	{
		Subsystem->NotifySnapshotChanged(Entry);
	}
}
