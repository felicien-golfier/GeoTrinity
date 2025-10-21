// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoGameMode.h"

#include "GeoPawn.h"

AGeoGameMode::AGeoGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = AGeoPawn::StaticClass();
}