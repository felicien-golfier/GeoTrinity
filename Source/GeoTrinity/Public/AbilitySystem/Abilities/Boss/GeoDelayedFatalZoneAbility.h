// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoDelayedFatalZoneAbility.generated.h"

/**
 * Boss ability that marks a random player's position with a countdown zone, then spawns a GeoPillar on expiry.
 * Picks the target location server-side using the seeded RNG, encodes it in the payload Origin so all clients
 * derive the same zone position deterministically via the FatalZonePattern.
 */
UCLASS()
class GEOTRINITY_API UGeoDelayedFatalZoneAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	virtual FVector2D GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
									  int Seed) const override;
};
