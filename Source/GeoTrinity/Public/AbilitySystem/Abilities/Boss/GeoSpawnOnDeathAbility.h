// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Characters/GeoEnemySetup.h"
#include "CoreMinimal.h"

#include "GeoSpawnOnDeathAbility.generated.h"

class AEnemyCharacter;
class AGeoCharacter;

/**
 * Passive of an add: once its owner dies, AllyClass spawns in its place as an add of the owner's arena, varied by
 * AllySetup. An owner destroyed without dying (arena wipe) leaves nothing.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnOnDeathAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoSpawnOnDeathAbility();

	/** Server. Starts listening for the owner's death; the ability never ends on its own. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Stops listening. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

protected:
	/** Spawns the ally when Character is this ability's owner. */
	void OnCharacterDied(AGeoCharacter& Character);

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|SpawnOnDeath")
	TSubclassOf<AEnemyCharacter> AllyClass;

	/** How the ally differs from AllyClass's defaults (team, tree, abilities, a tag to watch). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|SpawnOnDeath")
	FGeoEnemySetup AllySetup;
};
