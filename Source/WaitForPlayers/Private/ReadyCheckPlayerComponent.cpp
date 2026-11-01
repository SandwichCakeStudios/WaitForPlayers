// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckPlayerComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "ReadyCheckSubsystem.h"
#include "WaitForPlayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckPlayerComponent)

namespace
{
	const FReadyCheckPlayerEntry* FindEntryIn(const TArray<FReadyCheckPlayerEntry>& Entries, FGameplayTag CheckTag)
	{
		return Entries.FindByPredicate([CheckTag](const FReadyCheckPlayerEntry& Entry)
		{
			return Entry.CheckTag == CheckTag;
		});
	}

	UReadyCheckSubsystem* FindReadyCheckSubsystem(const UActorComponent& Component)
	{
		const UWorld* World = Component.GetWorld();
		return World != nullptr ? World->GetSubsystem<UReadyCheckSubsystem>() : nullptr;
	}
}

UReadyCheckPlayerComponent::UReadyCheckPlayerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UReadyCheckPlayerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UReadyCheckPlayerComponent, Entries, Params);
}

void UReadyCheckPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Covers PlayerStates that go away without a logout. The subsystem ignores players it already removed.
	if (EndPlayReason == EEndPlayReason::Destroyed && GetOwnerRole() == ROLE_Authority)
	{
		if (UReadyCheckSubsystem* Subsystem = FindReadyCheckSubsystem(*this))
		{
			if (APlayerState* Player = GetPlayerState<APlayerState>())
			{
				Subsystem->HandlePlayerLeft(*Player);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

const FReadyCheckPlayerEntry* UReadyCheckPlayerComponent::FindEntry(FGameplayTag CheckTag) const
{
	return FindEntryIn(Entries, CheckTag);
}

void UReadyCheckPlayerComponent::AddEntry(FGameplayTag CheckTag)
{
	FReadyCheckPlayerEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.CheckTag = CheckTag;
	MarkEntriesDirty();
}

void UReadyCheckPlayerComponent::RemoveEntry(FGameplayTag CheckTag)
{
	const int32 NumRemoved = Entries.RemoveAllSwap([CheckTag](const FReadyCheckPlayerEntry& Entry)
	{
		return Entry.CheckTag == CheckTag;
	});

	if (NumRemoved > 0)
	{
		MarkEntriesDirty();
	}
}

void UReadyCheckPlayerComponent::SetEntryReady(FGameplayTag CheckTag, bool bReady)
{
	for (FReadyCheckPlayerEntry& Entry : Entries)
	{
		if (Entry.CheckTag == CheckTag)
		{
			Entry.bReady = bReady;
			MarkEntriesDirty();
			return;
		}
	}
}

bool UReadyCheckPlayerComponent::TryConsumeRequest(FGameplayTag CheckTag, double Now, float Cooldown)
{
	for (FReadyCheckPlayerEntry& Entry : Entries)
	{
		if (Entry.CheckTag == CheckTag)
		{
			if (Entry.LastRequestTime >= 0.0 && Now - Entry.LastRequestTime < Cooldown)
			{
				return false;
			}

			// Not replicated, so this needs no dirty mark.
			Entry.LastRequestTime = Now;
			return true;
		}
	}

	return true;
}

void UReadyCheckPlayerComponent::ServerRequestReady_Implementation(FGameplayTag CheckTag, bool bReady)
{
	UReadyCheckSubsystem* Subsystem = FindReadyCheckSubsystem(*this);
	APlayerState* Player = GetPlayerState<APlayerState>();
	if (Subsystem == nullptr || Player == nullptr)
	{
		return;
	}

	const EReadyCheckResult Result = Subsystem->HandleReadyRequest(Player, CheckTag, bReady);
	if (Result != EReadyCheckResult::Success)
	{
		UE_LOG(LogWaitForPlayers, Verbose, TEXT("Rejected a ready request from %s for %s: %s"), *Player->GetPlayerName(), *CheckTag.ToString(), *UEnum::GetValueAsString(Result));
	}
}

void UReadyCheckPlayerComponent::OnRep_Entries(const TArray<FReadyCheckPlayerEntry>& OldEntries)
{
	UReadyCheckSubsystem* Subsystem = FindReadyCheckSubsystem(*this);
	APlayerState* Player = GetPlayerState<APlayerState>();
	if (Subsystem == nullptr || Player == nullptr)
	{
		return;
	}

	for (const FReadyCheckPlayerEntry& Entry : Entries)
	{
		const FReadyCheckPlayerEntry* OldEntry = FindEntryIn(OldEntries, Entry.CheckTag);
		if (Entry.bReady != (OldEntry != nullptr && OldEntry->bReady))
		{
			Subsystem->NotifyPlayerReadyChanged(Entry.CheckTag, Player, Entry.bReady);
		}
	}

	for (const FReadyCheckPlayerEntry& OldEntry : OldEntries)
	{
		if (OldEntry.bReady && FindEntryIn(Entries, OldEntry.CheckTag) == nullptr)
		{
			Subsystem->NotifyPlayerReadyChanged(OldEntry.CheckTag, Player, false);
		}
	}
}

void UReadyCheckPlayerComponent::MarkEntriesDirty()
{
	MARK_PROPERTY_DIRTY_FROM_NAME(UReadyCheckPlayerComponent, Entries, this);

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}
