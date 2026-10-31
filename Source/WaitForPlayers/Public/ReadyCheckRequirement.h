// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReadyCheckRequirement.generated.h"

/** The data a requirement can use to compute how many players must be ready. */
USTRUCT(BlueprintType)
struct FReadyCheckContext
{
	GENERATED_BODY()

	/** How many players are currently eligible under the policy. */
	UPROPERTY(BlueprintReadOnly, Category = "Ready Check")
	int32 NumEligible = 0;
};

/** Computes how many players must be ready. Subclass it in C++ or Blueprint to add a new rule. */
UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories)
class WAITFORPLAYERS_API UReadyCheckRequirement : public UObject
{
	GENERATED_BODY()

public:

	/** Returns the number of ready players the check needs. By default, every eligible player. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure = false, Category = "Ready Check")
	int32 ComputeRequired(const FReadyCheckContext& Context) const;
	virtual int32 ComputeRequired_Implementation(const FReadyCheckContext& Context) const;
};
