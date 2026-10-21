// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"

bool FSafeSpot::Shields(FVector2D const Target, FVector2D const Origin, FVector2D const Direction) const
{
	bool bShields = false;
	if (Shadow == ESafeSpotShadow::Lane)
	{
		FVector2D const Right(-Direction.Y, Direction.X);
		bShields = FMath::Abs((Target - Location) | Right) < Radius;
	}
	else
	{
		FVector2D const OriginToTarget(Target - Origin);
		FVector2D const OriginToSpot(Location - Origin);
		float const Dot = OriginToTarget | OriginToSpot;
		float const OriginToSpotSizeSquared = OriginToSpot.SizeSquared();

		if (Dot >= OriginToSpotSizeSquared) // Not before the spot
		{
			FVector2D const TargetProjectedOnOriginToSpot = OriginToSpot * Dot / OriginToSpotSizeSquared;
			bShields = (TargetProjectedOnOriginToSpot - OriginToTarget).SizeSquared() < Radius * Radius;
		}
	}

	return bShields;
}

bool FSafeSpot::AnyShields(TArray<FSafeSpot> const& Spots, FVector2D const Target, FVector2D const Origin,
						   FVector2D const Direction)
{
	return Spots.ContainsByPredicate(
		[&](FSafeSpot const& Spot)
		{
			return Spot.Shields(Target, Origin, Direction);
		});
}
