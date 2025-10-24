// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoGameMode.h"

#include "GeoInputGameInstanceSubsystem.h"
#include "GeoPawn.h"

AGeoGameMode::AGeoGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = AGeoPawn::StaticClass();
}
void AGeoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}