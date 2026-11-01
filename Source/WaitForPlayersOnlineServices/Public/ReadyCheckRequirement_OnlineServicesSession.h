// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ReadyCheckRequirement.h"
#include "ReadyCheckRequirement_OnlineServicesSession.generated.h"

/**
 * Every member of the online session must be ready, so the check waits for players still connecting.
 * Uses Online Services (v2). Without that session, it falls back to every eligible player.
 * Spectators in the session count too, so set bCountSpectators on the policy if your session has spectators.
 */
UCLASS(meta = (DisplayName = "Online Session Players (Online Services)"))
class WAITFORPLAYERSONLINESERVICES_API UReadyCheckRequirement_OnlineServicesSession : public UReadyCheckRequirement
{
	GENERATED_BODY()

public:

	virtual int32 ComputeRequired_Implementation(const FReadyCheckContext& Context) const override;
	virtual float GetRefreshInterval_Implementation() const override;

	/** The local name of the session to read. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement")
	FName SessionName = FName(TEXT("GameSession"));

	/** How often to read the session, in seconds, to notice changes that raise no roster event. 0 means only on roster events. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirement", meta = (ClampMin = "0.0", Units = "s"))
	float RefreshInterval = 1.f;
};
