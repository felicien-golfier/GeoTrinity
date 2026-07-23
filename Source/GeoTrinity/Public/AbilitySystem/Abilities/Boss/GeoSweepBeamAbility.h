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

public:
	/** Returns the sweep arc angle in degrees fed to the BeamPattern for this ability. */
	float GetSweepAngle() const { return Angle; }

protected:
	/** Yaw from Instigator toward the arena center; falls back to its facing when it is not a hex arena boss. */
	virtual float GetFireYaw(AActor const* Instigator) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Pattern", meta = (AllowPrivateAccess = "true"))
	float Angle = 60.f;
};
