// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeMoveToTask.h"

#include "STTask_MoveTo.generated.h"

/**
 * Instance data for FSTTask_MoveTo — extends the base Move To data with the gameplay cue played for the duration of
 * the move, which the task hands to the spawned UGeoAITask_MoveTo. Appends fields only, so base Move To logic is
 * unaffected.
 */
USTRUCT()
struct GEOTRINITY_API FSTTask_MoveToInstanceData : public FStateTreeMoveToTaskInstanceData
{
	GENERATED_BODY()

	/** Cue added when the move starts and removed when it ends, whether it arrived or was interrupted. Its
	 *  RawMagnitude carries the path length in world units, so the notify can scale playback to the distance. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "GameplayCue"))
	FGameplayTag MoveGameplayCueTag;
};

/**
 * StateTree Move To task that replans around dynamic obstacles (e.g. pillars) when the nav mesh changes.
 * Uses UGeoAITask_MoveTo instead of the base UAITask_MoveTo to enable path recalculation on invalidation,
 * and forwards MoveGameplayCueTag to it so every machine plays the move's cosmetics.
 */
USTRUCT(DisplayName = "Move To (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_MoveTo : public FStateTreeMoveToTask
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_MoveToInstanceData;

	/** Declares FSTTask_MoveToInstanceData as the per-execution instance data type so StateTree allocates the correct struct for each context. */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Spawns a UGeoAITask_MoveTo (nav-mesh recalculation on invalidation) and passes it the move cue tag. */
	virtual UAITask_MoveTo* PrepareMoveToTask(FStateTreeExecutionContext& Context, AAIController& Controller,
	                                          UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) const override;
};
