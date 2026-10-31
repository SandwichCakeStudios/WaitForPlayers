// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ReadyCheckTypes.generated.h"

/** The stages of a ready check. */
UENUM(BlueprintType)
enum class EReadyCheckPhase : uint8
{
	/** Never started, or cleared. */
	Inactive,
	/** Collecting ready requests. */
	Waiting,
	/** Requirement met, the post-ready countdown is running. The policy may skip this phase. */
	Countdown,
	/** Ended in success, or was forced to complete. */
	Completed,
	/** The timeout was reached and the policy's TimeoutAction is Fail. */
	TimedOut,
	/** Cancelled by the server, or by dropping below MinPlayers. */
	Cancelled
};

/** The outcome of every call that tries to change a ready check. */
UENUM(BlueprintType)
enum class EReadyCheckResult : uint8
{
	Success,
	/** Called on a client where the server is required. */
	NotAuthority,
	/** The tag is invalid, the policy is null, or no such check is running. */
	InvalidCheck,
	/** A check with this tag is already running. */
	AlreadyRunning,
	/** The player is null, a spectator, or a bot when the policy excludes it. */
	InvalidPlayer,
	/** The policy forbids this call, for example client requests or unready are off. */
	NotAllowed,
	/** The player sent another request before RequestCooldown elapsed. */
	RateLimited
};

/** What happens when a check runs out of time. */
UENUM(BlueprintType)
enum class EReadyCheckTimeoutAction : uint8
{
	/** End the check as TimedOut. */
	Fail,
	/** End the check as Completed, with bForced set. */
	Complete
};

/** The replicated state of one ready check, as seen by the server and every client. */
USTRUCT(BlueprintType)
struct FReadyCheckSnapshot
{
	GENERATED_BODY()

	/** The tag that identifies this check. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	FGameplayTag CheckTag;

	/** The current phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	EReadyCheckPhase Phase = EReadyCheckPhase::Inactive;

	/** How many eligible players are ready. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	int32 NumReady = 0;

	/** How many ready players the requirement asks for. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	int32 NumRequired = 0;

	/** When the current phase ends, in server world time. 0 means no deadline. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	double PhaseEndServerTime = 0.0;

	/** True if the check completed through ForceCompleteReadyCheck or TimeoutAction Complete. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	bool bForced = false;
};
