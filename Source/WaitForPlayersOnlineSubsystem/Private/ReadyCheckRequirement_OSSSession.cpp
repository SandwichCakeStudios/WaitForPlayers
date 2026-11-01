// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckRequirement_OSSSession.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "WaitForPlayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckRequirement_OSSSession)

int32 UReadyCheckRequirement_OSSSession::ComputeRequired_Implementation(const FReadyCheckContext& Context) const
{
	const IOnlineSessionPtr Sessions = Online::GetSessionInterface(Context.World.Get());
	const FNamedOnlineSession* Session = Sessions.IsValid() ? Sessions->GetNamedSession(SessionName) : nullptr;
	if (Session == nullptr)
	{
		UE_LOG(LogWaitForPlayers, Verbose, TEXT("No online session named %s. Requiring every eligible player."), *SessionName.ToString());
		return Context.NumEligible;
	}

	// Bots can be eligible without being registered.
	return FMath::Max(Session->RegisteredPlayers.Num(), Context.NumEligible);
}

float UReadyCheckRequirement_OSSSession::GetRefreshInterval_Implementation() const
{
	return RefreshInterval;
}
