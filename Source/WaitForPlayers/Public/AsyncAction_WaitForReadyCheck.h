// Copyright 2026 Sandwich Cake Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ReadyCheckTypes.h"
#include "AsyncAction_WaitForReadyCheck.generated.h"

class UReadyCheckSubsystem;
class UWorld;

/** The output pins of the async node. They share one type because the node takes its data pins from a single signature. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReadyCheckAsyncOutput, const FReadyCheckSnapshot&, Snapshot);

/**
 * Waits for a ready check to end. Works on the server and on clients.
 * The node stops listening when the check ends or the world is cleaned up.
 */
UCLASS()
class WAITFORPLAYERS_API UAsyncAction_WaitForReadyCheck : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	/**
	 * Waits for the check with this tag to end, and reports its progress until then.
	 * If the check has already ended, the matching output fires immediately.
	 * A client that has not received the check yet keeps waiting for it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ready Check", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Wait For Ready Check"))
	static UAsyncAction_WaitForReadyCheck* WaitForReadyCheck(const UObject* WorldContextObject, FGameplayTag CheckTag);

	//~ Begin UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase interface

	/** Fires when the check ends in Completed. The snapshot's bForced tells a forced completion apart. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FReadyCheckAsyncOutput OnCompleted;

	/** Fires when the check ends in TimedOut. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FReadyCheckAsyncOutput OnTimedOut;

	/** Fires when the check ends in Cancelled. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FReadyCheckAsyncOutput OnCancelled;

	/** Fires once when the node runs if the check is running, then whenever its counts or deadline change. */
	UPROPERTY(BlueprintAssignable, Category = "Ready Check")
	FReadyCheckAsyncOutput OnProgress;

private:

	void HandlePhaseChanged(const FReadyCheckSnapshot& Snapshot);
	void HandleProgressChanged(const FReadyCheckSnapshot& Snapshot);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	/** Fires the output that matches an ended check, and returns false if the check has not ended. */
	bool TryFinish(const FReadyCheckSnapshot& Snapshot);
	void StopListening();

	/** The subsystem of the node's world. Reset when the node stops listening, so a valid pointer means it is still waiting. */
	TWeakObjectPtr<UReadyCheckSubsystem> Subsystem;

	FGameplayTag CheckTag;

	FDelegateHandle PhaseChangedHandle;
	FDelegateHandle ProgressChangedHandle;
	FDelegateHandle WorldCleanupHandle;
};
