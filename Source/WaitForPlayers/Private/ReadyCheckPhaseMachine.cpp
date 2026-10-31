// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckPhaseMachine.h"
#include "ReadyCheckPolicy.h"

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

FReadyCheckSnapshot ReadyCheckPhaseMachine::Evaluate(const FReadyCheckSnapshot& Entry, const UReadyCheckPolicy& Policy, int32 EligibleCount, double Now)
{
	if (Entry.Phase != EReadyCheckPhase::Waiting)
	{
		return Entry;
	}

	// A check with nobody required stays open, so an empty server never completes by itself.
	if (Entry.NumRequired > 0 && Entry.NumReady >= Entry.NumRequired)
	{
		return End(Entry, EReadyCheckPhase::Completed, false);
	}

	if (Entry.PhaseEndServerTime > 0.0 && Now >= Entry.PhaseEndServerTime)
	{
		return End(Entry, EReadyCheckPhase::TimedOut, false);
	}

	return Entry;
}

FReadyCheckSnapshot ReadyCheckPhaseMachine::End(const FReadyCheckSnapshot& Entry, EReadyCheckPhase EndPhase, bool bForced)
{
	FReadyCheckSnapshot Snapshot = Entry;
	Snapshot.Phase = EndPhase;
	Snapshot.PhaseEndServerTime = 0.0;
	Snapshot.bForced = bForced;
	return Snapshot;
}
