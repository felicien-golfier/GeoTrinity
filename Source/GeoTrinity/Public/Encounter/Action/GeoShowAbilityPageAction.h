// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayTagContainer.h"

#include "GeoShowAbilityPageAction.generated.h"

/**
 * Opens AbilityTag's page on the local player's HUD (character sheet, its details drawer). Placed after the unlock's
 * Cat line, so the page never pops up before the Cat speaks. Skipped: nothing.
 */
UCLASS()
class GEOTRINITY_API UGeoShowAbilityPageAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag;
};
