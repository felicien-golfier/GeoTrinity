// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"

bool FGeoAbilityTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Origin;
	Ar << Yaw;
	Ar << ServerSpawnTime;
	Ar << Seed;
	bOutSuccess = true;
	return true;
}
