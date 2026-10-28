// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoGemDrop.h"
#include "CoreMinimal.h"

#include "GeoTestGemDrop.generated.h"

/** Gem drop that counts its sounds instead of playing them. */
UCLASS(NotPlaceable, Transient)
class AGeoTestGemDrop : public AGeoGemDrop
{
	GENERATED_BODY()

public:
	int32 SpawnSoundCount = 0;
	int32 PickupSoundCount = 0;

protected:
	virtual void PlayDropSound(FGeoSoundEntry const& Entry, FVector const& /*Location*/) override
	{
		(&Entry == &SpawnSound ? SpawnSoundCount : PickupSoundCount)++;
	}
};
