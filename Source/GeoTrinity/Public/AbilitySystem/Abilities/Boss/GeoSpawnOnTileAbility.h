// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoSpawnOnTileAbility.generated.h"

class AGeoHexArena;

/**
 * Server-only boss ability that drops deployables onto arena tiles — turrets that shell the party, mines that arm and
 * burst. Deployables spawned this way are removed by the arena when the tile under them is destroyed, so clearing the
 * ground is the counterplay to all of them.
 * With bSpawnOnHealthScaledRing the spawn ring walks in from the rim as the boss loses health; without it the tiles are
 * drawn from anywhere still standing.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnOnTileAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoSpawnOnTileAbility();

protected:
	/** Server. Spawns up to SpawnCount deployables on picked tiles, then ends the ability. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Ring to spawn on, scaled by the boss's remaining health, or negative to draw from the whole platform. */
	int32 GetSpawnRing(AGeoHexArena const& Arena) const;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|TileSpawn")
	TSubclassOf<AGeoDeployableBase> DeployableClass;

	/** Size, blink and life-drain of the spawned deployable — the drain duration is what arms a mine's fuse. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|TileSpawn")
	FDeployableDataParams DeployableParams;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|TileSpawn", meta = (ClampMin = "1"))
	int32 SpawnCount = 1;

	/** Spawns on the ring matching the boss's remaining health: the outer ring at full health, the center near death.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|TileSpawn")
	bool bSpawnOnHealthScaledRing = false;
};
