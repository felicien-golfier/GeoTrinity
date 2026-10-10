// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "CoreMinimal.h"

#include "GeoRelayPassiveAbility.generated.h"

/**
 * Passive of the Relay Core: the holder's dash snaps toward their nearest deployable (one of their deploy ability's
 * class, so never a reload buff pickup) within SnapHalfAngle of the dash direction, and stops at it or after
 * MaxRangeMultiplier times the dash's range, whichever comes first. A dash with no direction (standing still) heads for
 * the nearest deployable, whatever its angle.
 */
UCLASS()
class GEOTRINITY_API UGeoRelayPassiveAbility : public UGeoCorePassiveAbility
{
	GENERATED_BODY()

protected:
	/** Listens to the holder's OnDashAiming on the server and the predicting client alike, which both aim from
	 *  replicated state. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) override;
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) override;

private:
	/** Aims the holder's dash from Start: when a living deployable of the holder qualifies, replaces Direction with the
	 *  way to it and caps Distance at its range. */
	void SnapDash(FVector const& Start, bool bHasDirection, FVector& Direction, float& Distance);

	/** Largest angle in degrees between the dash direction and a deployable for the dash to snap to it. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Relay", meta = (ClampMin = "0", ClampMax = "180"))
	float SnapHalfAngle = 30.f;

	/** The snapped dash goes at most this many times the dash's range; 1.2 is 20% further. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Relay", meta = (ClampMin = "0"))
	float MaxRangeMultiplier = 1.2f;
};
