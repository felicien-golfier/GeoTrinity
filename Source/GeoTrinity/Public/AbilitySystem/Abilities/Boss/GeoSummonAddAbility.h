// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "Characters/GeoEnemySetup.h"
#include "CoreMinimal.h"

#include "GeoSummonAddAbility.generated.h"

class AEnemyCharacter;

/**
 * Boss ability bringing an add into the fight: its zone pattern (PatternToLaunch, a telegraph in the Damage colour then
 * the hit) lands on a spot near a live player, then AddClass spawns there as an add of the caster's arena, varied by
 * AddSetup. A force-stopped pattern (wipe, boss death) brings nothing.
 */
UCLASS()
class GEOTRINITY_API UGeoSummonAddAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	/** A live player, picked from Seed, offset by up to SpotRadius; the caster's own place when no player lives. */
	virtual FVector2D GetFireOrigin2D(AActor* Instigator, UGeoAbilitySystemComponent* SourceASC,
									  int Seed) const override;

	/** Server. The hit landed: spawns AddClass with AddSetup on its spot, then ends the ability. */
	virtual void OnPatternEnd() override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Summon")
	TSubclassOf<AEnemyCharacter> AddClass;

	/** The add's variant (health, tree, abilities), so AddClass holds only its look and its own behaviour's params. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Summon")
	FGeoEnemySetup AddSetup;

	/** Largest distance between the picked player and the spot. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Summon", meta = (ClampMin = "0"))
	float SpotRadius = 400.f;
};
