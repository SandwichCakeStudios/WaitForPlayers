// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "AsyncAction_WaitForReadyCheck.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ReadyCheckPhaseMachine.h"
#include "ReadyCheckSubsystem.h"
#include "WaitForPlayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_WaitForReadyCheck)

UAsyncAction_WaitForReadyCheck* UAsyncAction_WaitForReadyCheck::WaitForReadyCheck(const UObject* WorldContextObject, FGameplayTag CheckTag)
{
	UAsyncAction_WaitForReadyCheck* Action = NewObject<UAsyncAction_WaitForReadyCheck>();
	Action->CheckTag = CheckTag;

	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		Action->Subsystem = World->GetSubsystem<UReadyCheckSubsystem>();
		Action->RegisterWithGameInstance(WorldContextObject);
	}

	return Action;
}

void UAsyncAction_WaitForReadyCheck::Activate()
{
	UReadyCheckSubsystem* ReadyCheckSubsystem = Subsystem.Get();
	if (ReadyCheckSubsystem == nullptr || !CheckTag.IsValid())
	{
		UE_LOG(LogWaitForPlayers, Warning, TEXT("Wait For Ready Check: nothing to wait for, the world has no ready check subsystem or the tag '%s' is invalid."), *CheckTag.ToString());
		Subsystem.Reset();
		SetReadyToDestroy();
		return;
	}

	PhaseChangedHandle = ReadyCheckSubsystem->OnPhaseChangedNative.AddUObject(this, &UAsyncAction_WaitForReadyCheck::HandlePhaseChanged);
	ProgressChangedHandle = ReadyCheckSubsystem->OnProgressChangedNative.AddUObject(this, &UAsyncAction_WaitForReadyCheck::HandleProgressChanged);

	// A world that tears down ends its checks silently, so no phase event would release this node.
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UAsyncAction_WaitForReadyCheck::HandleWorldCleanup);

	FReadyCheckSnapshot Snapshot;
	if (ReadyCheckSubsystem->GetSnapshot(CheckTag, Snapshot) && !TryFinish(Snapshot) && ReadyCheckPhaseMachine::IsRunning(Snapshot.Phase))
	{
		OnProgress.Broadcast(Snapshot);
	}
}

void UAsyncAction_WaitForReadyCheck::HandlePhaseChanged(const FReadyCheckSnapshot& Snapshot)
{
	// A broadcast that is under way can still reach this node after it stopped listening.
	if (Subsystem.IsValid() && Snapshot.CheckTag == CheckTag)
	{
		TryFinish(Snapshot);
	}
}

void UAsyncAction_WaitForReadyCheck::HandleProgressChanged(const FReadyCheckSnapshot& Snapshot)
{
	if (Subsystem.IsValid() && Snapshot.CheckTag == CheckTag)
	{
		OnProgress.Broadcast(Snapshot);
	}
}

void UAsyncAction_WaitForReadyCheck::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	const UReadyCheckSubsystem* ReadyCheckSubsystem = Subsystem.Get();
	if (ReadyCheckSubsystem == nullptr || ReadyCheckSubsystem->GetWorld() == World)
	{
		StopListening();
		SetReadyToDestroy();
	}
}

bool UAsyncAction_WaitForReadyCheck::TryFinish(const FReadyCheckSnapshot& Snapshot)
{
	FReadyCheckAsyncOutput* Output = nullptr;
	switch (Snapshot.Phase)
	{
	case EReadyCheckPhase::Completed:
		Output = &OnCompleted;
		break;
	case EReadyCheckPhase::TimedOut:
		Output = &OnTimedOut;
		break;
	case EReadyCheckPhase::Cancelled:
		Output = &OnCancelled;
		break;
	default:
		return false;
	}

	// Stops first, so a listener that restarts the check cannot fire this node a second time.
	StopListening();
	Output->Broadcast(Snapshot);
	SetReadyToDestroy();
	return true;
}

void UAsyncAction_WaitForReadyCheck::StopListening()
{
	if (UReadyCheckSubsystem* ReadyCheckSubsystem = Subsystem.Get())
	{
		ReadyCheckSubsystem->OnPhaseChangedNative.Remove(PhaseChangedHandle);
		ReadyCheckSubsystem->OnProgressChangedNative.Remove(ProgressChangedHandle);
	}

	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	Subsystem.Reset();
}
