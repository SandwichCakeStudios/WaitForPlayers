// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckPhaseMachine.h"
#include "ReadyCheckGameStateComponent.h"
#include "ReadyCheckPolicy.h"

namespace
{
	/** Keeps a deadline that was reached in the same frame from reading as no deadline at all. */
	constexpr double MinWaitingTimeLeft = 0.001;
}

bool ReadyCheckPhaseMachine::IsRunning(EReadyCheckPhase Phase)
{
	return Phase == EReadyCheckPhase::Waiting || Phase == EReadyCheckPhase::Countdown;
}

bool ReadyCheckPhaseMachine::AreEqual(const FReadyCheckSnapshot& A, const FReadyCheckSnapshot& B)
{
	return A.CheckTag == B.CheckTag
		&& A.Phase == B.Phase
		&& A.NumReady == B.NumReady
		&& A.NumRequired == B.NumRequired
		&& A.PhaseEndServerTime == B.PhaseEndServerTime
		&& A.bForced == B.bForced;
}

FReadyCheckSnapshot ReadyCheckPhaseMachine::Begin(FGameplayTag CheckTag, const UReadyCheckPolicy& Policy, double Now)
{
	FReadyCheckSnapshot Snapshot;
	Snapshot.CheckTag = CheckTag;
	Snapshot.Phase = EReadyCheckPhase::Waiting;
	Snapshot.PhaseEndServerTime = Policy.Timeout > 0.f ? Now + Policy.Timeout : 0.0;
	return Snapshot;
}

FReadyCheckSnapshot ReadyCheckPhaseMachine::Evaluate(const FReadyCheckSnapshot& Entry, const UReadyCheckPolicy& Policy, int32 EligibleCount, double Now, FReadyCheckPhaseMemory& Memory)
{
	if (!IsRunning(Entry.Phase))
	{
		return Entry;
	}

	const bool bHasMinPlayers = EligibleCount >= Policy.MinPlayers;
	if (bHasMinPlayers)
	{
		Memory.bMinPlayersReached = true;
	}
	else if (Memory.bMinPlayersReached)
	{
		return End(Entry, EReadyCheckPhase::Cancelled, false);
	}

	// A check with nobody required stays open, so an empty server never completes by itself.
	const bool bRequirementMet = bHasMinPlayers && Entry.NumRequired > 0 && Entry.NumReady >= Entry.NumRequired;
	const bool bDeadlineReached = Entry.PhaseEndServerTime > 0.0 && Now >= Entry.PhaseEndServerTime;

	if (Entry.Phase == EReadyCheckPhase::Countdown)
	{
		if (!bRequirementMet)
		{
			FReadyCheckSnapshot Snapshot = Entry;
			Snapshot.Phase = EReadyCheckPhase::Waiting;
			Snapshot.PhaseEndServerTime = Memory.WaitingTimeLeft > 0.0 ? Now + Memory.WaitingTimeLeft : 0.0;
			return Snapshot;
		}

		return bDeadlineReached ? End(Entry, EReadyCheckPhase::Completed, false) : Entry;
	}

	if (bRequirementMet)
	{
		if (Policy.PostReadyCountdown <= 0.f)
		{
			return End(Entry, EReadyCheckPhase::Completed, false);
		}

		Memory.WaitingTimeLeft = Entry.PhaseEndServerTime > 0.0 ? FMath::Max(Entry.PhaseEndServerTime - Now, MinWaitingTimeLeft) : 0.0;

		FReadyCheckSnapshot Snapshot = Entry;
		Snapshot.Phase = EReadyCheckPhase::Countdown;
		Snapshot.PhaseEndServerTime = Now + Policy.PostReadyCountdown;
		return Snapshot;
	}

	if (bDeadlineReached)
	{
		return Policy.TimeoutAction == EReadyCheckTimeoutAction::Complete
			? End(Entry, EReadyCheckPhase::Completed, true)
			: End(Entry, EReadyCheckPhase::TimedOut, false);
	}

	return Entry;
}

bool ReadyCheckPhaseMachine::Extend(FReadyCheckSnapshot& Entry, const UReadyCheckPolicy& Policy, float Seconds, FReadyCheckPhaseMemory& Memory)
{
	const FReadyCheckTimeoutExtensionSettings& Settings = Policy.TimeoutExtension;
	if (!Settings.bEnabled || Entry.Phase != EReadyCheckPhase::Waiting || Entry.PhaseEndServerTime <= 0.0 || Seconds <= 0.f)
	{
		return false;
	}

	if (Settings.MaxExtensions > 0 && Memory.NumExtensions >= Settings.MaxExtensions)
	{
		return false;
	}

	if (Settings.MaxTotalExtraTime > 0.f && Memory.TotalExtraTime + Seconds > Settings.MaxTotalExtraTime)
	{
		return false;
	}

	Entry.PhaseEndServerTime += Seconds;
	++Memory.NumExtensions;
	Memory.TotalExtraTime += Seconds;
	return true;
}

FReadyCheckSnapshot ReadyCheckPhaseMachine::End(const FReadyCheckSnapshot& Entry, EReadyCheckPhase EndPhase, bool bForced)
{
	FReadyCheckSnapshot Snapshot = Entry;
	Snapshot.Phase = EndPhase;
	Snapshot.PhaseEndServerTime = 0.0;
	Snapshot.bForced = bForced;
	return Snapshot;
}
