// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "Gem/GeoGemTypes.h"

#include "GeoUnequipGemsAction.generated.h"

/**
 * Empties every socket of Tier in the build the first local player plays (and the echo when it holds a gem of that
 * tier), then saves: the copies go back to free. The tutorial's Gems step opens with it, so the chip the player picks
 * is never one already slotted. Lasting by design: nothing to undo on cancel or skip.
 */
UCLASS()
class GEOTRINITY_API UGeoUnequipGemsAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	EGeoGemTier Tier = EGeoGemTier::Chip;
};
