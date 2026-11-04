// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Characters/GeoEnemySetup.h"
#include "CoreMinimal.h"

#include "GeoSpawnEnemyCharacterAbility.generated.h"

class AEnemyCharacter;
struct FGameplayEventData;

/**
 * Server-only ability that spawns EnemyClass on each location of its Target, as an add of the caster's arena varied by
 * Setup, then ends. Fired on demand, or by its owner dying (bSpawnOnOwnerDying). A variant is a Blueprint child holding
 * only data.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnEnemyCharacterAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, no replication, and no fire delay. */
	UGeoSpawnEnemyCharacterAbility();

	/** The spawn needs its caster's arena itself. */
	virtual bool RequiresArena() const override { return true; }

protected:
	/** Server. With bSpawnOnOwnerDying, listens for the owner's Event.Character.Dying. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Server. Spawns one enemy per target location, then ends the ability. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/** Activates the ability when its owner is dying. */
	void OnOwnerDying(FGameplayEventData const* EventData);

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|SpawnEnemy")
	TSubclassOf<AEnemyCharacter> EnemyClass;

	/** How the enemy differs from EnemyClass's defaults (team, tree, abilities, a tag to watch). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|SpawnEnemy")
	FGeoEnemySetup Setup;

	/** Activates on the owner's death, before it is dead: Target Self is where it stands. An owner destroyed without
	 * dying leaves nothing. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|SpawnEnemy")
	bool bSpawnOnOwnerDying = false;

private:
	FGameplayAbilitySpecHandle OwnSpecHandle;
};
