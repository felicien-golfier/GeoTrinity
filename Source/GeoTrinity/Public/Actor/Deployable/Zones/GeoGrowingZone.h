// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "GeoGrowingZone.generated.h"

/**
 * Effect zone whose radius grows from its spawn size (Data.Params.Size, never written after spawn) to MaxRadius over
 * GrowthSeconds, then stays: pressure that never stops building until its spawner goes (the tutorial's summoned
 * dummies). Every machine grows it from its own BeginPlay; the server judges with that radius, a client only draws it.
 */
UCLASS()
class GEOTRINITY_API AGeoGrowingZone : public AGeoEffectZone
{
	GENERATED_BODY()

public:
	/** Advances the growth and resizes the zone. */
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Notes the time the growth starts from. */
	virtual void BeginPlay() override;

	/** The spawn size grown toward MaxRadius by GrowthAlpha. */
	virtual float GetRadius() const override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoGrowingZone")
	FScalableFloat GrowthSeconds = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoGrowingZone")
	FScalableFloat MaxRadius = 600.f;

private:
	float StartTime = 0.f;

	/** 0 at the spawn size, 1 at MaxRadius. */
	float GrowthAlpha = 0.f;
};
