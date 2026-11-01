// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ReadyCheckTypes.h"

class UReadyCheckPolicy;
struct FReadyCheckPhaseMemory;

/** The phase rules of a ready check, as pure functions. They make no engine calls. */
namespace ReadyCheckPhaseMachine
{
	/** Returns true while a check can still change: Waiting or Countdown. */
	bool IsRunning(EReadyCheckPhase Phase);

	/** Returns true if both snapshots describe the same state. */
	bool AreEqual(const FReadyCheckSnapshot& A, const FReadyCheckSnapshot& B);

	/** Returns the state of a check that starts at Now. */
	FReadyCheckSnapshot Begin(FGameplayTag CheckTag, const UReadyCheckPolicy& Policy, double Now);

	/** Returns the state that follows Entry at Now. NumReady and NumRequired must be up to date. */
	FReadyCheckSnapshot Evaluate(const FReadyCheckSnapshot& Entry, const UReadyCheckPolicy& Policy, int32 EligibleCount, double Now, FReadyCheckPhaseMemory& Memory);

	/** Moves Entry's deadline forward by Seconds. Returns false and changes nothing if the policy refuses. */
	bool Extend(FReadyCheckSnapshot& Entry, const UReadyCheckPolicy& Policy, float Seconds, FReadyCheckPhaseMemory& Memory);

	/** Returns Entry ended in the given phase. */
	FReadyCheckSnapshot End(const FReadyCheckSnapshot& Entry, EReadyCheckPhase EndPhase, bool bForced);
}
