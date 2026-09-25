// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"

#include "GeoDeployTargetCue.generated.h"

class AGeoProjectile;
class UGeoDeployAbility;

/**
 * Looping cue marking where a deployable will land, added locally by UGeoDeployAbility in two stages:
 * - while charging, on the character with the ability as SourceObject: follows GetPendingDeployLocation() every tick;
 * - on release, on the spawned projectile at the landing point: holds still until that projectile ends.
 * Targeting the projectile gives each deploy in flight its own marker. Blueprint subclasses only author the tag and
 * the visual.
 */
UCLASS(Abstract)
class GEOTRINITY_API AGeoDeployTargetCue : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	/** Enables auto-destruction when the cue is removed. */
	AGeoDeployTargetCue();

	/** Snaps this actor to the deploy ability's current pending drop location. */
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** On a projectile target, waits for it to end; otherwise grabs the deploy ability from SourceObject and follows it. */
	virtual bool OnActive_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters) override;

	/** Stops following, so a recycled instance never moves while hidden. */
	virtual bool OnRemove_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters) override;

private:
	/** Ends the landing marker once its projectile has landed. */
	UFUNCTION()
	void OnLandingProjectileEnded(AGeoProjectile* Projectile);

	TWeakObjectPtr<UGeoDeployAbility const> DeployAbility;
};
