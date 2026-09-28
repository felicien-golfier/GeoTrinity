// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"

#include "GeoZoneIndicatorCue.generated.h"

/**
 * Round zone telegraph for a cue executed locally from state every machine already holds (a deployable's moments, a
 * pattern's InitCue): draws through UGeoIndicatorComponent::SpawnIndicator, in the cue's colour and secondary colours.
 * Reads RawMagnitude as the radius, Normal.X as the lifetime and Normal.Y as the time already elapsed. Attached to
 * SourceObject when it is an actor, so it rides a moving deployable.
 */
UCLASS()
class GEOTRINITY_API UGeoZoneIndicatorCue : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	/** Spawns the telegraph on this machine. */
	virtual bool OnExecute_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters) const override;
};
