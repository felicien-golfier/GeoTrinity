// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;

/** The way a safe spot shadows its hazard. The ordinals are the material scalar SafeSpot_ShadowMode: never reorder. */
enum class ESafeSpotShadow : uint8
{
	/** Radial, away from the hazard origin, starting at the spot (pillars). */
	Behind = 0,
	/** Strip of the spot's width parallel to the hazard's direction, both sides of the spot (lateral wave). */
	Lane = 1,
	/** The spot's own disc, nothing else (a round hole in a beam). */
	Disc = 2,
};

/** A disc that casts a shadow: a target inside the shadow is not hit. The one geometry behind the hit test and the VFX
 * mask of every hazard with safe spots. */
struct GEOTRINITY_API FSafeSpot
{
	FVector2D Location;
	float Radius = 0.f;
	ESafeSpotShadow Shadow = ESafeSpotShadow::Disc;
	/** The actor the spot stands for (pillar); null for any other. */
	TWeakObjectPtr<AActor> Source;

	/** True when Target lies in this spot's shadow. Origin is the hazard origin (Behind), Direction its unit direction
	 * (Lane); Disc reads neither. */
	bool Shields(FVector2D Target, FVector2D Origin, FVector2D Direction) const;

	/** True when any of Spots shields Target. */
	static bool AnyShields(TArray<FSafeSpot> const& Spots, FVector2D Target, FVector2D Origin, FVector2D Direction);
};
