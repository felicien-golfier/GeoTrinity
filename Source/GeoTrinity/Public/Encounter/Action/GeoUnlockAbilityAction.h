// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterAction.h"
#include "GameplayTagContainer.h"

#include "GeoUnlockAbilityAction.generated.h"

/**
 * Grants the local player's class ability AbilityTag, held back by the map's LockedAbilityTags, then opens its page
 * (character sheet, its details drawer). Already granted: nothing happens. Skipped: granted, no page.
 */
UCLASS()
class GEOTRINITY_API UGeoUnlockAbilityAction : public UGeoEncounterAction
{
	GENERATED_BODY()

public:
	virtual void Execute_Implementation(FGeoEncounterPayload const& Payload) override;
	virtual void OnSkipped_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag;

private:
	/** Grants AbilityTag to the local player's class; true when it was not granted yet. */
	bool GrantAbility() const;
};
