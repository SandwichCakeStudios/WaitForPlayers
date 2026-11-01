// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReadyCheckTypes.h"
#include "ReadyCheckPolicy.generated.h"

class UReadyCheckRequirement;

/** Optional: lets a running check gain extra time. */
USTRUCT(BlueprintType)
struct FReadyCheckTimeoutExtensionSettings
{
	GENERATED_BODY()

	/** Allow ExtendTimeout for this check. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timeout Extension")
	bool bEnabled = false;

	/** How many times a check can be extended. 0 means unlimited. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timeout Extension", meta = (EditCondition = "bEnabled", ClampMin = "0"))
	int32 MaxExtensions = 0;

	/** Cap on all extensions added together. 0 means unlimited. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timeout Extension", meta = (EditCondition = "bEnabled", ClampMin = "0.0", Units = "s"))
	float MaxTotalExtraTime = 0.f;

	/** When a new eligible player joins during Waiting, add AutoExtendSeconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timeout Extension", meta = (EditCondition = "bEnabled"))
	bool bAutoExtendOnPlayerJoin = false;

	/** Seconds added per join. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timeout Extension", meta = (EditCondition = "bEnabled && bAutoExtendOnPlayerJoin", ClampMin = "0.0", Units = "s"))
	float AutoExtendSeconds = 0.f;
};

/** Optional: lets a reconnecting player keep their ready state. */
USTRUCT(BlueprintType)
struct FReadyCheckReconnectSettings
{
	GENERATED_BODY()

	/** A reconnecting player keeps their ready state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reconnect")
	bool bEnabled = false;

	/** How long a player who left is remembered. 0 means for the whole check. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reconnect", meta = (EditCondition = "bEnabled", ClampMin = "0.0", Units = "s"))
	float GracePeriod = 0.f;

	/** Players who left within the grace period still count toward NumRequired, so the check waits for them. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reconnect", meta = (EditCondition = "bEnabled"))
	bool bHoldSlotWhileAway = false;
};

/** The designer-authored rules of a ready check. */
UCLASS(BlueprintType, Const)
class WAITFORPLAYERS_API UReadyCheckPolicy : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** How NumRequired is computed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Requirement")
	TObjectPtr<UReadyCheckRequirement> Requirement;

	/** The check cannot complete below this many eligible players. Once reached, dropping below it cancels the check. 0 turns this off. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requirement", meta = (ClampMin = "0"))
	int32 MinPlayers = 0;

	/** Whether bots are eligible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requirement")
	bool bCountBots = false;

	/** Whether spectators are eligible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requirement")
	bool bCountSpectators = false;

	/** Seconds in Waiting. 0 means no timeout. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float Timeout = 0.f;

	/** What happens when Timeout is reached. Complete ends the check as Completed with bForced set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing")
	EReadyCheckTimeoutAction TimeoutAction = EReadyCheckTimeoutAction::Fail;

	/** Seconds in Countdown before Completed. 0 skips the countdown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float PostReadyCountdown = 0.f;

	/** Clients may ready themselves. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requests")
	bool bAllowClientRequests = true;

	/** Players may withdraw their ready state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requests")
	bool bAllowUnready = true;

	/** An unready during Countdown returns the check to Waiting. When off, players cannot unready during Countdown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requests", meta = (EditCondition = "bAllowUnready"))
	bool bUnreadyRevertsCountdown = true;

	/** Minimum seconds between one player's requests, to stop spam. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requests", meta = (ClampMin = "0.0", Units = "s"))
	float RequestCooldown = 0.25f;

	/** Optional: lets a running check gain extra time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Optional Features")
	FReadyCheckTimeoutExtensionSettings TimeoutExtension;

	/** Optional: lets a reconnecting player keep their ready state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Optional Features")
	FReadyCheckReconnectSettings Reconnect;
};
