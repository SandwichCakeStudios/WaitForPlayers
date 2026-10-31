// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#include "ReadyCheckRequirement.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReadyCheckRequirement)

int32 UReadyCheckRequirement::ComputeRequired_Implementation(const FReadyCheckContext& Context) const
{
	return Context.NumEligible;
}
