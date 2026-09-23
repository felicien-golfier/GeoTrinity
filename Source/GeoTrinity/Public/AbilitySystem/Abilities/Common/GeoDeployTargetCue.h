// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"

#include "GeoDeployTargetCue.generated.h"

class UGeoDeployAbility;

/**
 * Looping cue marking where a charging deploy ability would land. Added locally by UGeoDeployAbility, which passes
 * itself as the cue's SourceObject; the actor follows GetPendingDeployLocation() every tick until removed.
 * Blueprint subclasses only author the tag and the visual.
 */
UCLASS(Abstract)
class GEOTRINITY_API AGeoDeployTargetCue : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGeoDeployTargetCue();

	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Grabs the deploy ability from SourceObject and starts following it. */
	virtual bool OnActive_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters) override;

	/** Stops following, so a recycled instance never moves while hidden. */
	virtual bool OnRemove_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters) override;

private:
	TWeakObjectPtr<UGeoDeployAbility const> DeployAbility;
};
