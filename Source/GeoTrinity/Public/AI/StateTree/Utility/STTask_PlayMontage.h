// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_PlayMontage.generated.h"

class UAnimMontage;
class UGeoAbilitySystemComponent;

/** Per-instance data for FSTTask_PlayMontage (StateTree instance data pattern). */
USTRUCT()
struct GEOTRINITY_API FSTTask_PlayMontageInstanceData
{
	GENERATED_BODY()

	/** Montage played on the pawn when the state is entered. */
	UPROPERTY(EditAnywhere, Category = "GeoParameter")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** Speed multiplier applied to the montage. */
	UPROPERTY(EditAnywhere, Category = "GeoParameter", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;
};

/**
 * Plays a montage through the pawn's ASC, which replicates it to every client — the tree runs on the server only, so
 * a bare Montage_Play would be seen by nobody else. Runs until the montage ends, then Succeeds.
 */
USTRUCT(DisplayName = "Play Montage", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_PlayMontage : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_PlayMontageInstanceData;

	/** Disables tick; completion is driven by the montage-end delegate. */
	FSTTask_PlayMontage();

	/** Returns FSTTask_PlayMontageInstanceData as the per-execution instance data type. */
	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Plays the montage on the ASC and binds its end delegate for async completion. Fails if the montage is not set,
	 * the ASC is missing, or the montage could not be played. */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;

	/** Stops the montage when the state is left before it ended. */
	virtual void ExitState(FStateTreeExecutionContext& Context,
						   FStateTreeTransitionResult const& Transition) const override;

private:
	UGeoAbilitySystemComponent* GetASC(FStateTreeExecutionContext const& Context) const;
};
