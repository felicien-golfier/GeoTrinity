// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"

bool FGeoAbilityTargetData_Orientation::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Origin;
	Ar << Yaw;
	bOutSuccess = true;
	return true;
}
