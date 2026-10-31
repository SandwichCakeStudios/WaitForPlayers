// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ReadyCheckPolicy.h"
#include "WaitForPlayers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckSubsystem)

// The checks themselves arrive with the core implementation. Until then no check can run,
// so calls that need one report InvalidCheck.

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

	UE_LOG(LogWaitForPlayers, Warning, TEXT("StartReadyCheck: not implemented yet, %s was not started."), *CheckTag.ToString());
	return EReadyCheckResult::NotAllowed;
}

EReadyCheckResult UReadyCheckSubsystem::CancelReadyCheck(FGameplayTag CheckTag)
{
	return HasAuthority() ? EReadyCheckResult::InvalidCheck : EReadyCheckResult::NotAuthority;
}

EReadyCheckResult UReadyCheckSubsystem::ForceCompleteReadyCheck(FGameplayTag CheckTag)
{
	return HasAuthority() ? EReadyCheckResult::InvalidCheck : EReadyCheckResult::NotAuthority;
}

EReadyCheckResult UReadyCheckSubsystem::SetPlayerReady(APlayerState* Player, FGameplayTag CheckTag, bool bReady)
{
	if (!HasAuthority())
	{
		return EReadyCheckResult::NotAuthority;
	}

	return Player != nullptr ? EReadyCheckResult::InvalidCheck : EReadyCheckResult::InvalidPlayer;
}

EReadyCheckResult UReadyCheckSubsystem::ExtendTimeout(FGameplayTag CheckTag, float Seconds)
{
	return HasAuthority() ? EReadyCheckResult::InvalidCheck : EReadyCheckResult::NotAuthority;
}

EReadyCheckResult UReadyCheckSubsystem::RequestLocalPlayerReady(const APlayerController* LocalController, FGameplayTag CheckTag, bool bReady)
{
	if (LocalController == nullptr || !LocalController->IsLocalController())
	{
		return EReadyCheckResult::InvalidPlayer;
	}

	return EReadyCheckResult::InvalidCheck;
}

bool UReadyCheckSubsystem::GetSnapshot(FGameplayTag CheckTag, FReadyCheckSnapshot& OutSnapshot) const
{
	OutSnapshot = FReadyCheckSnapshot();
	return false;
}

bool UReadyCheckSubsystem::IsPlayerReady(const APlayerState* Player, FGameplayTag CheckTag) const
{
	return false;
}

void UReadyCheckSubsystem::GetPlayers(FGameplayTag CheckTag, bool bReady, TArray<APlayerState*>& OutPlayers) const
{
	OutPlayers.Reset();
}

float UReadyCheckSubsystem::GetRemainingTime(FGameplayTag CheckTag) const
{
	return 0.f;
}

void UReadyCheckSubsystem::GetActiveChecks(TArray<FGameplayTag>& OutCheckTags) const
{
	OutCheckTags.Reset();
}

bool UReadyCheckSubsystem::HasAuthority() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->GetNetMode() != NM_Client;
}
