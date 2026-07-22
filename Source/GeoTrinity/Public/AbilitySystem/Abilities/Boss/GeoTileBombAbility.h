// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoTileBombAbility.generated.h"

/**
 * Server-only boss ability that drops a bomb deployable on one live player and attaches it to them. The bomb rides the
 * carrier through its life-drain wind-up, then plants where they stand and detonates after its blink — destroying the
 * tiles underneath and hitting whoever is caught. The carrier decides where the hole lands: walk it to the rim, or lose
 * the middle of the platform. See AGeoBombZone for the two-phase drain/blink behaviour.
 */
UCLASS()
class GEOTRINITY_API UGeoTileBombAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoTileBombAbility();

protected:
	/** Server. Draws one live player from the payload seed, spawns the bomb on them and attaches it, then ends. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/** Bomb dropped on the carrier — an AGeoBombZone whose drain/blink durations drive the two wind-up phases. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|TileBomb")
	TSubclassOf<AGeoDeployableBase> BombClass;

	/** Blast radius (Size), wind-up duration (LifeDrainMaxDuration) and planted-telegraph duration (BlinkDuration). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|TileBomb")
	FDeployableDataParams BombParams;
};
