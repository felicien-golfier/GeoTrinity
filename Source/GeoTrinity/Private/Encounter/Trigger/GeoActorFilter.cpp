// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoActorFilter.h"

#include "Characters/GeoCharacter.h"

bool FGeoActorFilter::Matches(AActor const& Actor) const
{
	return Class && Actor.IsA(Class) && (Tag.IsNone() || Actor.ActorHasTag(Tag));
}
