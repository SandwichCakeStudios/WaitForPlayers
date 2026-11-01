// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckRequirement.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckRequirement)

int32 UReadyCheckRequirement::ComputeRequired_Implementation(const FReadyCheckContext& Context) const
{
	return Context.NumEligible;
}

float UReadyCheckRequirement::GetRefreshInterval_Implementation() const
{
	return 0.f;
}

int32 UReadyCheckRequirement_FixedCount::ComputeRequired_Implementation(const FReadyCheckContext& Context) const
{
	return Count;
}

int32 UReadyCheckRequirement_Percentage::ComputeRequired_Implementation(const FReadyCheckContext& Context) const
{
	// Multiply first, so whole percentages stay exact: 70% of 10 players must be 7, not 8.
	return FMath::CeilToInt32(static_cast<double>(Percentage) * Context.NumEligible / 100.0);
}
