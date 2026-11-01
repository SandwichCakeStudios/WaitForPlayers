// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckRequirement_OnlineServicesSession.h"
#include "Online/OnlineServices.h"
#include "Online/Sessions.h"
#include "WaitForPlayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckRequirement_OnlineServicesSession)

int32 UReadyCheckRequirement_OnlineServicesSession::ComputeRequired_Implementation(const FReadyCheckContext& Context) const
{
	using namespace UE::Online;

	const IOnlineServicesPtr Services = GetServices(EOnlineServices::Default);
	const ISessionsPtr Sessions = Services.IsValid() ? Services->GetSessionsInterface() : nullptr;
	if (Sessions.IsValid())
	{
		FGetSessionByName::Params Params;
		Params.LocalName = SessionName;

		const TOnlineResult<FGetSessionByName> Result = Sessions->GetSessionByName(MoveTemp(Params));
		if (Result.IsOk())
		{
			// Bots can be eligible without being session members.
			return FMath::Max(Result.GetOkValue().Session->GetSessionMembers().Num(), Context.NumEligible);
		}
	}

	UE_LOG(LogWaitForPlayers, Verbose, TEXT("No online session named %s. Requiring every eligible player."), *SessionName.ToString());
	return Context.NumEligible;
}

float UReadyCheckRequirement_OnlineServicesSession::GetRefreshInterval_Implementation() const
{
	return RefreshInterval;
}
