// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.h"
#include "Characters/GeoEnemySetup.h"
#include "CoreMinimal.h"

#include "SpawnAddPattern.generated.h"

class AEnemyCharacter;

/**
 * Zone pattern whose hit brings an add: once it ended naturally, AddClass spawns on each target location as an add of
 * the caster's arena, varied by AddSetup. A force-stopped pattern (wipe, boss death) brings nothing.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API USpawnAddPattern : public UZonePattern
{
	GENERATED_BODY()

public:
	/** Brings the add into its caster's arena. */
	virtual bool RequiresArena() const override { return true; }

	/** Server, natural end: spawns the adds before Super, whose end broadcast ends the ability. */
	virtual void EndPattern(bool bForceStop = false) override;

protected:
	/** Flags a missing AddClass so a misconfigured pattern is caught at creation time. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoSummon")
	TSubclassOf<AEnemyCharacter> AddClass;

	/** The add's variant (health, tree, abilities), so AddClass holds only its look and its own behaviour's params. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoSummon")
	FGeoEnemySetup AddSetup;
};
