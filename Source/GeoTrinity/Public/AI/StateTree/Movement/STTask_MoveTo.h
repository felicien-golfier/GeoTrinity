// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeMoveToTask.h"

#include "STTask_MoveTo.generated.h"

class UCurveFloat;
class USoundBase;

/**
 * Instance data for FSTTask_MoveTo — extends the base Move To data with a looping move sound and its pitch curve,
 * which the task hands to the spawned UGeoAITask_MoveTo. Appends fields only, so base Move To logic is unaffected.
 */
USTRUCT()
struct GEOTRINITY_API FSTTask_MoveToInstanceData : public FStateTreeMoveToTaskInstanceData
{
	GENERATED_BODY()

	/** Looping sound played for the whole move; its pitch follows MovePitchCurve across the travel time. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<USoundBase> MoveSound;

	/** Pitch multiplier for MoveSound sampled at the 0..1 move progress (X=0 start, X=1 arrival). Pitch stays 1 when unset. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UCurveFloat> MovePitchCurve;
};

/**
 * StateTree Move To task that replans around dynamic obstacles (e.g. pillars) when the nav mesh changes.
 * Uses UGeoAITask_MoveTo instead of the base UAITask_MoveTo to enable path recalculation on invalidation,
 * and forwards MoveSound / MovePitchCurve to it so the enemy plays a distance-scaled move sound.
 */
USTRUCT(DisplayName = "Move To (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_MoveTo : public FStateTreeMoveToTask
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_MoveToInstanceData;

	/** Declares FSTTask_MoveToInstanceData as the per-execution instance data type so StateTree allocates the correct struct for each context. */
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Spawns a UGeoAITask_MoveTo (nav-mesh recalculation on invalidation) and passes it the move sound + pitch curve. */
	virtual UAITask_MoveTo* PrepareMoveToTask(FStateTreeExecutionContext& Context, AAIController& Controller,
	                                          UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) const override;
};
