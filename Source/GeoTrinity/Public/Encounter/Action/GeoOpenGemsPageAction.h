// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"

#include "GeoOpenGemsPageAction.generated.h"

/** Opens the Gems menu on the local player's HUD, on its Forge page or its loadout. */
UCLASS()
class GEOTRINITY_API UGeoOpenGemsPageAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** True: the Forge page; false: the loadout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	bool bForge = false;
};
