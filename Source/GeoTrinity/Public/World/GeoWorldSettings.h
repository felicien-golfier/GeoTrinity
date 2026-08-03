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
	/** Fills GameDataSettings::ColorPaletteCollection from ColorPalette — the world-scoped hook every material reading
	 * a semantic color depends on (collection values live on the UWorld, so they reset with each level). */
	virtual void BeginPlay() override;

	/** When set to anything other than None, all players start with this class regardless of slot order. */
	UPROPERTY(EditAnywhere, Category = "GeoTrinity")
	EPlayerClass StartingClassOverride = EPlayerClass::None;
};
