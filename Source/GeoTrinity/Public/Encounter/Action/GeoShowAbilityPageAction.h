// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayTagContainer.h"

#include "GeoShowAbilityPageAction.generated.h"

/**
 * Opens AbilityTag's page on the local player's HUD (character sheet, its details drawer). Placed after the unlock's
 * Cat line, so the page never pops up before the Cat speaks. Latent: finishes when the player closes the pause menu
 * and comes back to the game, so what follows never plays behind the page. Skipped: nothing.
 */
UCLASS()
class GEOTRINITY_API UGeoShowAbilityPageAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	UGeoShowAbilityPageAction() { bLatent = true; }

	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;

	/** Stops waiting for the close; the page stays open until the player closes it. */
	virtual void OnCancel_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag;

private:
	/** Bumped on cancel: the close of a page opened by a cancelled run no longer finishes the action. */
	int32 PageRun = 0;
};
