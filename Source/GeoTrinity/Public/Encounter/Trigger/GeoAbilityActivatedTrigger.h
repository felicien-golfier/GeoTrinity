// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTrigger.h"
#include "GameplayTagContainer.h"

#include "GeoAbilityActivatedTrigger.generated.h"

class UGameplayAbility;
class UGeoAbilitySystemComponent;

/**
 * Fires when the first local player activates the ability tagged AbilityTag. Count = activations since arming, so an
 * activation before the step never counts.
 */
UCLASS()
class GEOTRINITY_API UGeoAbilityActivatedTrigger : public UGeoEncounterTrigger
{
	GENERATED_BODY()

public:
	/** Binds to the first local player's ASC ability-activated delegate. */
	virtual void OnArm_Implementation() override;
	/** Unbinds the ability-activated delegate. */
	virtual void OnDisarm_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (Categories = "Ability.Spell"))
	FGameplayTag AbilityTag;

private:
	void OnAbilityActivated(UGameplayAbility* Ability);

	TWeakObjectPtr<UGeoAbilitySystemComponent> WatchedASC;
	int32 Activations = 0;
};
