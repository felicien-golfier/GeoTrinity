// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_SetInvulnerable.generated.h"

class AGeoCharacter;

/** Per-instance data for FSTTask_SetInvulnerable (StateTree instance data pattern). */
USTRUCT()
struct GEOTRINITY_API FSTTask_SetInvulnerableInstanceData
{
	GENERATED_BODY()

	/** Applied to the pawn when the state is entered; its opposite when the state is left. */
	UPROPERTY(EditAnywhere, Category = "GeoParameter")
	bool bInvulnerable = true;
};

/**
 * Holds the pawn invulnerable — no effect lands on it, no collision touches it — for as long as the state is active,
 * and hands it back its normal state on exit. Runs on the server only, like the whole tree; the pawn replicates it.
 */
USTRUCT(DisplayName = "Set Invulnerable", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_SetInvulnerable : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_SetInvulnerableInstanceData;

	/** Disables tick; the task only acts on the state's entry and exit. */
	FSTTask_SetInvulnerable();

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Sets the pawn's invulnerability and keeps running so the state's other tasks decide when it ends. Fails when
	 * the owner is not an AGeoCharacter. */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;

	/** Restores the opposite invulnerability, whatever ended the state. */
	virtual void ExitState(FStateTreeExecutionContext& Context,
						   FStateTreeTransitionResult const& Transition) const override;

private:
	AGeoCharacter* GetCharacter(FStateTreeExecutionContext const& Context) const;
};
