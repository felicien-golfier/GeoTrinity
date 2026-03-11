// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoGameMode.h"

#include "Characters/GeoCharacter.h"

AGeoGameMode::AGeoGameMode(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	DefaultPawnClass = AGeoCharacter::StaticClass();
}
void AGeoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
