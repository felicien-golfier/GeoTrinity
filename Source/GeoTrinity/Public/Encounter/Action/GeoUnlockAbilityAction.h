// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayTagContainer.h"

#include "GeoUnlockAbilityAction.generated.h"

/**
 * Grants the local player's class ability AbilityTag, held back by the map's LockedAbilityTags. Already granted:
 * nothing happens. Never opens the page (UGeoShowAbilityPageAction, after the Cat's line). Skipped: granted.
 */
UCLASS()
class GEOTRINITY_API UGeoUnlockAbilityAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	/** Grants the local player's class ability for AbilityTag; does nothing when already granted. */
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;
	/** Grants the ability as Execute would, so a skipped unlock still reaches the player. */
	virtual void OnSkipped_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag;
};
