// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoSweepBeamAbility.generated.h"

/**
 * Boss ability that aims its beam pattern at the arena center instead of straight ahead, so the sweep always crosses
 * the middle of the platform and the ground behind the boss stays the safe side.
 */
UCLASS()
class GEOTRINITY_API UGeoSweepBeamAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	/** Yaw from Instigator toward the arena center; falls back to its facing when it is not a hex arena boss. */
	virtual float GetFireYaw(AActor const* Instigator) const override;
};
