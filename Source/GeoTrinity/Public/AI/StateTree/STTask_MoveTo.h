// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeMoveToTask.h"

#include "STTask_MoveTo.generated.h"

/**
 * StateTree Move To task that replans around dynamic obstacles (e.g. pillars) when the nav mesh changes.
 * Uses UGeoAITask_MoveTo instead of the base UAITask_MoveTo to enable path recalculation on invalidation.
 */
USTRUCT(DisplayName = "Move To (Geo)", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_MoveTo : public FStateTreeMoveToTask
{
	GENERATED_BODY()

	virtual UAITask_MoveTo* PrepareMoveToTask(FStateTreeExecutionContext& Context, AAIController& Controller,
	                                          UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) const override;
};
