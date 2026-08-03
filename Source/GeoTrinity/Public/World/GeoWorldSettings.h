// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"

#include "GeoWorldSettings.generated.h"

UCLASS()
class GEOTRINITY_API AGeoWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	/** When set to anything other than None, all players start with this class regardless of slot order. */
	UPROPERTY(EditAnywhere, Category = "GeoTrinity")
	EPlayerClass StartingClassOverride = EPlayerClass::None;
};
