// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounterObject.h"

#include "Encounter/GeoEncounter.h"

UWorld* UGeoEncounterObject::GetWorld() const
{
	UObject const* const Outer = GetOuter();
	if (HasAnyFlags(RF_ClassDefaultObject) || !Outer)
	{
		return nullptr;
	}
	return Outer->GetWorld();
}

AGeoEncounter* UGeoEncounterObject::GetEncounter() const
{
	return GetTypedOuter<AGeoEncounter>();
}
