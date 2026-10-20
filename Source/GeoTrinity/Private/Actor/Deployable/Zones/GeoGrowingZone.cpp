// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/Zones/GeoGrowingZone.h"

#include "Engine/World.h"

void AGeoGrowingZone::BeginPlay()
{
	Super::BeginPlay();

	StartTime = GetWorld()->GetTimeSeconds();
}

float AGeoGrowingZone::GetRadius() const
{
	return FMath::Lerp(Data.Params.Size, MaxRadius.GetValueAtLevel(Data.Level), GrowthAlpha);
}

void AGeoGrowingZone::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float const Seconds = GrowthSeconds.GetValueAtLevel(Data.Level);
	float const Alpha =
		Seconds > 0.f ? FMath::Clamp((GetWorld()->GetTimeSeconds() - StartTime) / Seconds, 0.f, 1.f) : 1.f;
	if (Alpha != GrowthAlpha)
	{
		GrowthAlpha = Alpha;
		ApplyRadius();
	}
}
