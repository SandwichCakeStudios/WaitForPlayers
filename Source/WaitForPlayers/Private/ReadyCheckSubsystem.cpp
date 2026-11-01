// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ReadyCheckGameStateComponent.h"
#include "ReadyCheckPhaseMachine.h"
#include "ReadyCheckPlayerComponent.h"
#include "ReadyCheckPolicy.h"
#include "ReadyCheckRequirement.h"
#include "TimerManager.h"
#include "WaitForPlayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckSubsystem)

namespace
{
	using FCheckTagList = TArray<FGameplayTag, TInlineAllocator<4>>;

	/** A timer set to 0 would not fire, and a held slot's timer can run slightly early. */
	constexpr float MinAwayTimerDelay = 0.001f;

	/** Listeners can start or end checks, so loops that raise events work on a copy of the tags. */
	FCheckTagList GetRunningChecks(const UReadyCheckGameStateComponent& Store)
	{
		FCheckTagList CheckTags;
		for (const FReadyCheckEntry& Entry : Store.GetEntries())
		{
			if (ReadyCheckPhaseMachine::IsRunning(Entry.Snapshot.Phase))
			{
				CheckTags.Add(Entry.Snapshot.CheckTag);
			}
		}
		return CheckTags;
	}

	bool IsEligible(const APlayerState& Player, const UReadyCheckPolicy& Policy)
	{
		if (Player.IsABot() && !Policy.bCountBots)
		{
			return false;
		}

		return Policy.bCountSpectators || !(Player.IsSpectator() || Player.IsOnlyASpectator());
	}

	int32 ComputeRequired(const UReadyCheckPolicy& Policy, int32 NumEligible, UWorld* World)
	{
		if (Policy.Requirement == nullptr)
		{
			return NumEligible;
		}

		FReadyCheckContext Context;
		Context.NumEligible = NumEligible;
		Context.World = World;
		return FMath::Max(Policy.Requirement->ComputeRequired(Context), 0);
	}

	/** The players a check counts: everyone in it, plus held slots of players who may come back. */
	int32 GetNumCountedPlayers(const FReadyCheckServerState& State)
	{
		const FReadyCheckReconnectSettings& Reconnect = State.Policy->Reconnect;
		return State.NumEligible + (Reconnect.bEnabled && Reconnect.bHoldSlotWhileAway ? State.AwayPlayers.Num() : 0);
	}

	void RemoveExpiredAwayPlayers(FReadyCheckServerState& State, double Now)
	{
		const float GracePeriod = State.Policy->Reconnect.GracePeriod;
		if (GracePeriod <= 0.f)
		{
			return;
		}

		for (auto It = State.AwayPlayers.CreateIterator(); It; ++It)
		{
			if (Now >= It.Value().LeaveServerTime + GracePeriod)
			{
				It.RemoveCurrent();
			}
		}
	}

	/** Uses the component authored on the actor, or adds one at runtime. Replication then creates it on clients. */
	template<typename ComponentType>
	ComponentType* FindOrInjectComponent(AActor& Actor)
	{
		if (ComponentType* Component = Actor.FindComponentByClass<ComponentType>())
		{
			return Component;
		}

		ComponentType* Component = NewObject<ComponentType>(&Actor);
		Component->RegisterComponent();
		return Component;
	}
}

void UReadyCheckSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Static events, shared by every world. The handlers ignore the other worlds.
	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UReadyCheckSubsystem::HandlePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UReadyCheckSubsystem::HandleLogout);
}

void UReadyCheckSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::Deinitialize();
}

bool UReadyCheckSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

EReadyCheckResult UReadyCheckSubsystem::StartReadyCheck(FGameplayTag CheckTag, const UReadyCheckPolicy* Policy)
{
	if (!HasAuthority())
	{
		return EReadyCheckResult::NotAuthority;
	}

	if (!CheckTag.IsValid() || Policy == nullptr)
	{
		return EReadyCheckResult::InvalidCheck;
	}

	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState == nullptr)
	{
		UE_LOG(LogWaitForPlayers, Warning, TEXT("StartReadyCheck: %s was not started because the world has no GameState yet."), *CheckTag.ToString());
		return EReadyCheckResult::NotAllowed;
	}

	UReadyCheckGameStateComponent* Store = FindOrInjectComponent<UReadyCheckGameStateComponent>(*GameState);
	FReadyCheckEntry& Entry = Store->FindOrAddEntry(CheckTag);
	if (ReadyCheckPhaseMachine::IsRunning(Entry.Snapshot.Phase))
	{
		return EReadyCheckResult::AlreadyRunning;
	}

	FReadyCheckServerState& State = Store->FindOrAddServerState(CheckTag);
	State.Policy = Policy;
	State.NumEligible = 0;
	State.PhaseMemory = FReadyCheckPhaseMemory();

	const double Now = GetServerTime();
	Entry.Snapshot = ReadyCheckPhaseMachine::Begin(CheckTag, *Policy, Now);

	for (APlayerState* Player : GameState->PlayerArray)
	{
		if (Player == nullptr)
		{
			continue;
		}

		// A restart discards the previous run of this tag.
		if (UReadyCheckPlayerComponent* PlayerComponent = Player->FindComponentByClass<UReadyCheckPlayerComponent>())
		{
			PlayerComponent->RemoveEntry(CheckTag);
		}

		UpdateMembership(*Player, Entry.Snapshot, State);
	}

	// Armed first, because the check can end during its first evaluation.
	ArmRefreshTimer(CheckTag, State);
	ReevaluateCheck(CheckTag, Now);
	return EReadyCheckResult::Success;
}

EReadyCheckResult UReadyCheckSubsystem::CancelReadyCheck(FGameplayTag CheckTag)
{
	if (!HasAuthority())
	{
		return EReadyCheckResult::NotAuthority;
	}

	UReadyCheckGameStateComponent* Store = GetStore();
	FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	if (Entry == nullptr || !ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase))
	{
		return EReadyCheckResult::InvalidCheck;
	}

	CommitSnapshot(*Store, *Entry, *Store->FindServerState(CheckTag), ReadyCheckPhaseMachine::End(Entry->Snapshot, EReadyCheckPhase::Cancelled, false));
	return EReadyCheckResult::Success;
}

EReadyCheckResult UReadyCheckSubsystem::ForceCompleteReadyCheck(FGameplayTag CheckTag)
{
	if (!HasAuthority())
	{
		return EReadyCheckResult::NotAuthority;
	}

	UReadyCheckGameStateComponent* Store = GetStore();
	FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	if (Entry == nullptr || !ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase))
	{
		return EReadyCheckResult::InvalidCheck;
	}

	CommitSnapshot(*Store, *Entry, *Store->FindServerState(CheckTag), ReadyCheckPhaseMachine::End(Entry->Snapshot, EReadyCheckPhase::Completed, true));
	return EReadyCheckResult::Success;
}

EReadyCheckResult UReadyCheckSubsystem::SetPlayerReady(APlayerState* Player, FGameplayTag CheckTag, bool bReady)
{
	if (!HasAuthority())
	{
		return EReadyCheckResult::NotAuthority;
	}

	return ApplyPlayerReady(Player, CheckTag, bReady);
}

EReadyCheckResult UReadyCheckSubsystem::ExtendTimeout(FGameplayTag CheckTag, float Seconds)
{
	if (!HasAuthority())
	{
		return EReadyCheckResult::NotAuthority;
	}

	UReadyCheckGameStateComponent* Store = GetStore();
	FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	if (Entry == nullptr || !ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase))
	{
		return EReadyCheckResult::InvalidCheck;
	}

	FReadyCheckServerState& State = *Store->FindServerState(CheckTag);
	if (!ReadyCheckPhaseMachine::Extend(Entry->Snapshot, *State.Policy, Seconds, State.PhaseMemory))
	{
		return EReadyCheckResult::NotAllowed;
	}

	ReevaluateCheck(CheckTag, GetServerTime());
	return EReadyCheckResult::Success;
}

EReadyCheckResult UReadyCheckSubsystem::RequestLocalPlayerReady(const APlayerController* LocalController, FGameplayTag CheckTag, bool bReady)
{
	if (LocalController == nullptr || !LocalController->IsLocalController())
	{
		return EReadyCheckResult::InvalidPlayer;
	}

	APlayerState* Player = LocalController->GetPlayerState<APlayerState>();
	if (Player == nullptr)
	{
		return EReadyCheckResult::InvalidPlayer;
	}

	// The listen-server host follows the same rules as every other player.
	if (HasAuthority())
	{
		return HandleReadyRequest(Player, CheckTag, bReady);
	}

	FReadyCheckSnapshot Snapshot;
	if (!GetSnapshot(CheckTag, Snapshot) || !ReadyCheckPhaseMachine::IsRunning(Snapshot.Phase))
	{
		return EReadyCheckResult::InvalidCheck;
	}

	// The component can replicate after the check does. Until then this client cannot send requests.
	UReadyCheckPlayerComponent* PlayerComponent = Player->FindComponentByClass<UReadyCheckPlayerComponent>();
	if (PlayerComponent == nullptr)
	{
		return EReadyCheckResult::InvalidPlayer;
	}

	PlayerComponent->ServerRequestReady(CheckTag, bReady);
	return EReadyCheckResult::Success;
}

bool UReadyCheckSubsystem::GetSnapshot(FGameplayTag CheckTag, FReadyCheckSnapshot& OutSnapshot) const
{
	const UReadyCheckGameStateComponent* Store = GetStore();
	const FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	if (Entry == nullptr)
	{
		OutSnapshot = FReadyCheckSnapshot();
		return false;
	}

	OutSnapshot = Entry->Snapshot;
	return true;
}

bool UReadyCheckSubsystem::IsPlayerReady(const APlayerState* Player, FGameplayTag CheckTag) const
{
	const UReadyCheckPlayerComponent* PlayerComponent = Player != nullptr ? Player->FindComponentByClass<UReadyCheckPlayerComponent>() : nullptr;
	const FReadyCheckPlayerEntry* PlayerEntry = PlayerComponent != nullptr ? PlayerComponent->FindEntry(CheckTag) : nullptr;
	return PlayerEntry != nullptr && PlayerEntry->bReady;
}

void UReadyCheckSubsystem::GetPlayers(FGameplayTag CheckTag, bool bReady, TArray<APlayerState*>& OutPlayers) const
{
	OutPlayers.Reset();

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World != nullptr ? World->GetGameState() : nullptr;
	if (GameState == nullptr)
	{
		return;
	}

	for (APlayerState* Player : GameState->PlayerArray)
	{
		const UReadyCheckPlayerComponent* PlayerComponent = Player != nullptr ? Player->FindComponentByClass<UReadyCheckPlayerComponent>() : nullptr;
		const FReadyCheckPlayerEntry* PlayerEntry = PlayerComponent != nullptr ? PlayerComponent->FindEntry(CheckTag) : nullptr;
		if (PlayerEntry != nullptr && PlayerEntry->bReady == bReady)
		{
			OutPlayers.Add(Player);
		}
	}
}

float UReadyCheckSubsystem::GetRemainingTime(FGameplayTag CheckTag) const
{
	FReadyCheckSnapshot Snapshot;
	if (!GetSnapshot(CheckTag, Snapshot) || Snapshot.PhaseEndServerTime <= 0.0)
	{
		return 0.f;
	}

	return FMath::Max(static_cast<float>(Snapshot.PhaseEndServerTime - GetServerTime()), 0.f);
}

void UReadyCheckSubsystem::GetActiveChecks(TArray<FGameplayTag>& OutCheckTags) const
{
	OutCheckTags.Reset();

	if (const UReadyCheckGameStateComponent* Store = GetStore())
	{
		for (const FReadyCheckEntry& Entry : Store->GetEntries())
		{
			OutCheckTags.Add(Entry.Snapshot.CheckTag);
		}
	}
}

bool UReadyCheckSubsystem::HasAuthority() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->GetNetMode() != NM_Client;
}

double UReadyCheckSubsystem::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World != nullptr ? World->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->GetServerWorldTimeSeconds() : 0.0;
}

UReadyCheckGameStateComponent* UReadyCheckSubsystem::GetStore() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World != nullptr ? World->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->FindComponentByClass<UReadyCheckGameStateComponent>() : nullptr;
}

EReadyCheckResult UReadyCheckSubsystem::HandleReadyRequest(APlayerState* Player, FGameplayTag CheckTag, bool bReady)
{
	if (Player == nullptr)
	{
		return EReadyCheckResult::InvalidPlayer;
	}

	UReadyCheckGameStateComponent* Store = GetStore();
	const FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	if (Entry == nullptr || !ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase))
	{
		return EReadyCheckResult::InvalidCheck;
	}

	const UReadyCheckPolicy& Policy = *Store->FindServerState(CheckTag)->Policy;
	if (!Policy.bAllowClientRequests)
	{
		return EReadyCheckResult::NotAllowed;
	}

	if (!bReady && (!Policy.bAllowUnready || (Entry->Snapshot.Phase == EReadyCheckPhase::Countdown && !Policy.bUnreadyRevertsCountdown)))
	{
		return EReadyCheckResult::NotAllowed;
	}

	UReadyCheckPlayerComponent* PlayerComponent = Player->FindComponentByClass<UReadyCheckPlayerComponent>();
	if (PlayerComponent != nullptr && !PlayerComponent->TryConsumeRequest(CheckTag, GetServerTime(), Policy.RequestCooldown))
	{
		return EReadyCheckResult::RateLimited;
	}

	return ApplyPlayerReady(Player, CheckTag, bReady);
}

EReadyCheckResult UReadyCheckSubsystem::ApplyPlayerReady(APlayerState* Player, FGameplayTag CheckTag, bool bReady)
{
	if (Player == nullptr)
	{
		return EReadyCheckResult::InvalidPlayer;
	}

	UReadyCheckGameStateComponent* Store = GetStore();
	FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	if (Entry == nullptr || !ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase))
	{
		return EReadyCheckResult::InvalidCheck;
	}

	// Eligibility can change without a roster event, for example when a player starts spectating.
	UpdateMembership(*Player, Entry->Snapshot, *Store->FindServerState(CheckTag));

	UReadyCheckPlayerComponent* PlayerComponent = Player->FindComponentByClass<UReadyCheckPlayerComponent>();
	const FReadyCheckPlayerEntry* PlayerEntry = PlayerComponent != nullptr ? PlayerComponent->FindEntry(CheckTag) : nullptr;
	const bool bIsMember = PlayerEntry != nullptr;

	if (bIsMember && PlayerEntry->bReady != bReady)
	{
		PlayerComponent->SetEntryReady(CheckTag, bReady);
		Entry->Snapshot.NumReady += bReady ? 1 : -1;
		NotifyPlayerReadyChanged(CheckTag, Player, bReady);
	}

	ReevaluateCheck(CheckTag, GetServerTime());
	return bIsMember ? EReadyCheckResult::Success : EReadyCheckResult::InvalidPlayer;
}

void UReadyCheckSubsystem::HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (GameMode == nullptr || GameMode->GetWorld() != GetWorld() || NewPlayer == nullptr)
	{
		return;
	}

	if (APlayerState* Player = NewPlayer->GetPlayerState<APlayerState>())
	{
		HandlePlayerJoined(*Player);
	}
}

void UReadyCheckSubsystem::HandleLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (GameMode == nullptr || GameMode->GetWorld() != GetWorld() || Exiting == nullptr)
	{
		return;
	}

	if (APlayerState* Player = Exiting->GetPlayerState<APlayerState>())
	{
		HandlePlayerLeft(*Player);
	}
}

void UReadyCheckSubsystem::HandlePlayerJoined(APlayerState& Player)
{
	UReadyCheckGameStateComponent* Store = GetStore();
	if (Store == nullptr)
	{
		return;
	}

	for (const FGameplayTag& CheckTag : GetRunningChecks(*Store))
	{
		FReadyCheckEntry* Entry = Store->FindEntry(CheckTag);
		if (Entry != nullptr && ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase))
		{
			FReadyCheckServerState& State = *Store->FindServerState(CheckTag);
			const bool bNewMember = UpdateMembership(Player, Entry->Snapshot, State);
			const bool bReturned = State.Policy->Reconnect.bEnabled && RestoreAwayPlayer(Player, *Entry, State);

			// RestoreAwayPlayer can raise events, so Entry and State are only safe when it found nobody.
			if (bNewMember && !bReturned && State.Policy->TimeoutExtension.bAutoExtendOnPlayerJoin)
			{
				ReadyCheckPhaseMachine::Extend(Entry->Snapshot, *State.Policy, State.Policy->TimeoutExtension.AutoExtendSeconds, State.PhaseMemory);
			}

			ReevaluateCheck(CheckTag, GetServerTime());
		}
	}
}

void UReadyCheckSubsystem::HandlePlayerLeft(APlayerState& Player)
{
	// A world that is tearing down ends its checks silently.
	const UWorld* World = GetWorld();
	if (World == nullptr || World->bIsTearingDown)
	{
		return;
	}

	UReadyCheckGameStateComponent* Store = GetStore();
	UReadyCheckPlayerComponent* PlayerComponent = Player.FindComponentByClass<UReadyCheckPlayerComponent>();
	if (Store == nullptr || PlayerComponent == nullptr)
	{
		return;
	}

	for (const FGameplayTag& CheckTag : GetRunningChecks(*Store))
	{
		FReadyCheckEntry* Entry = Store->FindEntry(CheckTag);
		const FReadyCheckPlayerEntry* PlayerEntry = PlayerComponent->FindEntry(CheckTag);
		if (Entry != nullptr && ReadyCheckPhaseMachine::IsRunning(Entry->Snapshot.Phase) && PlayerEntry != nullptr)
		{
			FReadyCheckServerState& State = *Store->FindServerState(CheckTag);
			if (State.Policy->Reconnect.bEnabled)
			{
				RememberAwayPlayer(Player, PlayerEntry->bReady, CheckTag, State);
			}

			RemoveMember(*PlayerComponent, Entry->Snapshot, State);
			ReevaluateCheck(CheckTag, GetServerTime());
		}
	}
}

bool UReadyCheckSubsystem::UpdateMembership(APlayerState& Player, FReadyCheckSnapshot& Snapshot, FReadyCheckServerState& State)
{
	UReadyCheckPlayerComponent* PlayerComponent = Player.FindComponentByClass<UReadyCheckPlayerComponent>();
	const bool bIsMember = PlayerComponent != nullptr && PlayerComponent->FindEntry(Snapshot.CheckTag) != nullptr;
	const bool bEligible = IsEligible(Player, *State.Policy);

	if (bEligible && !bIsMember)
	{
		FindOrInjectComponent<UReadyCheckPlayerComponent>(Player)->AddEntry(Snapshot.CheckTag);
		++State.NumEligible;
		return true;
	}

	if (!bEligible && bIsMember)
	{
		RemoveMember(*PlayerComponent, Snapshot, State);
	}

	return false;
}

void UReadyCheckSubsystem::RemoveMember(UReadyCheckPlayerComponent& PlayerComponent, FReadyCheckSnapshot& Snapshot, FReadyCheckServerState& State)
{
	const FReadyCheckPlayerEntry* PlayerEntry = PlayerComponent.FindEntry(Snapshot.CheckTag);
	if (PlayerEntry != nullptr && PlayerEntry->bReady)
	{
		--Snapshot.NumReady;
	}

	PlayerComponent.RemoveEntry(Snapshot.CheckTag);
	--State.NumEligible;
}

void UReadyCheckSubsystem::RememberAwayPlayer(const APlayerState& Player, bool bReady, FGameplayTag CheckTag, FReadyCheckServerState& State)
{
	const FUniqueNetIdRepl& NetId = Player.GetUniqueId();
	if (Player.IsABot() || !NetId.IsValid())
	{
		return;
	}

	const double Now = GetServerTime();
	RemoveExpiredAwayPlayers(State, Now);

	FReadyCheckAwayPlayer& AwayPlayer = State.AwayPlayers.Add(NetId);
	AwayPlayer.bReady = bReady;
	AwayPlayer.LeaveServerTime = Now;

	ArmAwayTimer(CheckTag, State);
}

bool UReadyCheckSubsystem::RestoreAwayPlayer(APlayerState& Player, FReadyCheckEntry& Entry, FReadyCheckServerState& State)
{
	const FGameplayTag CheckTag = Entry.Snapshot.CheckTag;
	RemoveExpiredAwayPlayers(State, GetServerTime());

	const FUniqueNetIdRepl& NetId = Player.GetUniqueId();
	FReadyCheckAwayPlayer AwayPlayer;
	const bool bFound = NetId.IsValid() && State.AwayPlayers.RemoveAndCopyValue(NetId, AwayPlayer);
	ArmAwayTimer(CheckTag, State);

	if (!bFound)
	{
		return false;
	}

	UReadyCheckPlayerComponent* PlayerComponent = Player.FindComponentByClass<UReadyCheckPlayerComponent>();
	const FReadyCheckPlayerEntry* PlayerEntry = PlayerComponent != nullptr ? PlayerComponent->FindEntry(CheckTag) : nullptr;
	if (AwayPlayer.bReady && PlayerEntry != nullptr && !PlayerEntry->bReady)
	{
		PlayerComponent->SetEntryReady(CheckTag, true);
		++Entry.Snapshot.NumReady;
		NotifyPlayerReadyChanged(CheckTag, &Player, true);
	}

	return true;
}

void UReadyCheckSubsystem::ForgetAwayPlayers(FReadyCheckServerState& State)
{
	GetWorld()->GetTimerManager().ClearTimer(State.AwayTimer);
	State.AwayPlayers.Empty();
}

void UReadyCheckSubsystem::ArmAwayTimer(FGameplayTag CheckTag, FReadyCheckServerState& State)
{
	const FReadyCheckReconnectSettings& Reconnect = State.Policy->Reconnect;
	if (!Reconnect.bHoldSlotWhileAway || Reconnect.GracePeriod <= 0.f)
	{
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(State.AwayTimer);

	if (State.AwayPlayers.Num() > 0)
	{
		double EarliestLeaveTime = TNumericLimits<double>::Max();
		for (const TPair<FUniqueNetIdRepl, FReadyCheckAwayPlayer>& Pair : State.AwayPlayers)
		{
			EarliestLeaveTime = FMath::Min(EarliestLeaveTime, Pair.Value.LeaveServerTime);
		}

		const float Delay = FMath::Max(static_cast<float>(EarliestLeaveTime + Reconnect.GracePeriod - GetServerTime()), MinAwayTimerDelay);
		const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UReadyCheckSubsystem::HandleAwayExpiry, CheckTag);
		TimerManager.SetTimer(State.AwayTimer, Delegate, Delay, false);
	}
}

void UReadyCheckSubsystem::HandleAwayExpiry(FGameplayTag CheckTag)
{
	UReadyCheckGameStateComponent* Store = GetStore();
	FReadyCheckServerState* State = Store != nullptr ? Store->FindServerState(CheckTag) : nullptr;
	if (State != nullptr)
	{
		RemoveExpiredAwayPlayers(*State, GetServerTime());
		ArmAwayTimer(CheckTag, *State);
		ReevaluateCheck(CheckTag, GetServerTime());
	}
}

void UReadyCheckSubsystem::ReevaluateCheck(FGameplayTag CheckTag, double Now)
{
	UReadyCheckGameStateComponent* Store = GetStore();
	FReadyCheckEntry* Entry = Store != nullptr ? Store->FindEntry(CheckTag) : nullptr;
	FReadyCheckServerState* State = Store != nullptr ? Store->FindServerState(CheckTag) : nullptr;
	if (Entry == nullptr || State == nullptr)
	{
		return;
	}

	FReadyCheckSnapshot NewSnapshot = Entry->Snapshot;
	const int32 NumCounted = GetNumCountedPlayers(*State);
	NewSnapshot.NumRequired = ComputeRequired(*State->Policy, NumCounted, GetWorld());
	CommitSnapshot(*Store, *Entry, *State, ReadyCheckPhaseMachine::Evaluate(NewSnapshot, *State->Policy, NumCounted, Now, State->PhaseMemory));
}

void UReadyCheckSubsystem::CommitSnapshot(UReadyCheckGameStateComponent& Store, FReadyCheckEntry& Entry, FReadyCheckServerState& State, const FReadyCheckSnapshot& NewSnapshot)
{
	Entry.Snapshot = NewSnapshot;

	// Before the early return: a restart can end exactly like the previous run, which raises no change.
	if (!ReadyCheckPhaseMachine::IsRunning(NewSnapshot.Phase))
	{
		GetWorld()->GetTimerManager().ClearTimer(State.RefreshTimer);
	}

	if (ReadyCheckPhaseMachine::AreEqual(NewSnapshot, Entry.LastNotified))
	{
		return;
	}

	if (NewSnapshot.PhaseEndServerTime != Entry.LastNotified.PhaseEndServerTime)
	{
		ArmPhaseTimer(NewSnapshot, State);
	}

	if (!ReadyCheckPhaseMachine::IsRunning(NewSnapshot.Phase) && State.AwayPlayers.Num() > 0)
	{
		ForgetAwayPlayers(State);
	}

	Store.MarkEntryDirty(Entry);
	NotifySnapshotChanged(Entry);
}

void UReadyCheckSubsystem::ArmPhaseTimer(const FReadyCheckSnapshot& Snapshot, FReadyCheckServerState& State)
{
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(State.PhaseTimer);

	if (Snapshot.PhaseEndServerTime > 0.0)
	{
		const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UReadyCheckSubsystem::HandlePhaseDeadline, Snapshot.CheckTag);
		TimerManager.SetTimer(State.PhaseTimer, Delegate, static_cast<float>(Snapshot.PhaseEndServerTime - GetServerTime()), false);
	}
}

void UReadyCheckSubsystem::HandlePhaseDeadline(FGameplayTag CheckTag)
{
	FReadyCheckSnapshot Snapshot;
	if (GetSnapshot(CheckTag, Snapshot))
	{
		// Timers and world time add up separately. The deadline must count as reached when the timer fires.
		ReevaluateCheck(CheckTag, FMath::Max(GetServerTime(), Snapshot.PhaseEndServerTime));
	}
}

void UReadyCheckSubsystem::ArmRefreshTimer(FGameplayTag CheckTag, FReadyCheckServerState& State)
{
	const float Interval = State.Policy->Requirement != nullptr ? State.Policy->Requirement->GetRefreshInterval() : 0.f;
	if (Interval > 0.f)
	{
		const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UReadyCheckSubsystem::HandleRefresh, CheckTag);
		GetWorld()->GetTimerManager().SetTimer(State.RefreshTimer, Delegate, Interval, true);
	}
}

void UReadyCheckSubsystem::HandleRefresh(FGameplayTag CheckTag)
{
	ReevaluateCheck(CheckTag, GetServerTime());
}

void UReadyCheckSubsystem::NotifySnapshotChanged(FReadyCheckEntry& Entry)
{
	// Listeners can start checks, which moves the entries. Past this point only the copies are safe.
	const FReadyCheckSnapshot Previous = Entry.LastNotified;
	const FReadyCheckSnapshot Current = Entry.Snapshot;
	Entry.LastNotified = Current;

	const bool bProgressChanged = Current.NumReady != Previous.NumReady
		|| Current.NumRequired != Previous.NumRequired
		|| Current.PhaseEndServerTime != Previous.PhaseEndServerTime;

	if (bProgressChanged)
	{
		OnProgressChangedNative.Broadcast(Current);
		OnProgressChanged.Broadcast(Current);
	}

	if (Current.Phase != Previous.Phase)
	{
		OnPhaseChangedNative.Broadcast(Current);
		OnPhaseChanged.Broadcast(Current);
	}
}

void UReadyCheckSubsystem::NotifyPlayerReadyChanged(FGameplayTag CheckTag, APlayerState* Player, bool bReady)
{
	OnPlayerReadyChangedNative.Broadcast(CheckTag, Player, bReady);
	OnPlayerReadyChanged.Broadcast(CheckTag, Player, bReady);
}
