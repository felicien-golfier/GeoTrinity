// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/AITask_MoveTo.h"

#include "GeoAITask_MoveTo.generated.h"

/**
 * AITask_MoveTo variant that enables path recalculation on nav mesh invalidation.
 * The base class disables auto-repath, which prevents the enemy from replanning around dynamic obstacles
 * (e.g. pillars) that appear after the initial path is computed. This subclass re-enables it.
 */
UCLASS()
class GEOTRINITY_API UGeoAITask_MoveTo : public UAITask_MoveTo
{
	GENERATED_BODY()

protected:
	virtual void PerformMove() override;
};
