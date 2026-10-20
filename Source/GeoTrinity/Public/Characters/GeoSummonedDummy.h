// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/GeoEnemySetup.h"
#include "CoreMinimal.h"

#include "GeoSummonedDummy.generated.h"

class AGeoGrowingZone;

/**
 * Add a boss summons (GeoSummonAddAbility, tutorial miniboss). A zone grows at its feet for as long as it lives and
 * dies with it; killed, it leaves an ally of AllyClass in its place, an add of the same arena.
 */
UCLASS()
class GEOTRINITY_API AGeoSummonedDummy : public AEnemyCharacter
{
	GENERATED_BODY()

protected:
	/** Server. Spawns its zone at its feet. */
	virtual void BeginPlay() override;
	/** Server. Spawns AllyClass in its place through its arena. */
	virtual void DeathLogic() override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoSummonedDummy")
	TSubclassOf<AGeoGrowingZone> ZoneClass;

	/** What the zone does to those inside: damage per second. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoSummonedDummy")
	TArray<TInstancedStruct<FEffectData>> ZoneEffects;

	/** Start size (radius), colour and attitude of the zone; its life drain stays 0, so it lasts as long as the dummy.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GeoSummonedDummy")
	FDeployableDataParams ZoneParams;

	/** What a killed dummy turns into. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoSummonedDummy")
	TSubclassOf<AEnemyCharacter> AllyClass;

	/** How the ally differs from AllyClass's defaults (player team, its own tree and abilities, a tag to watch). */
	UPROPERTY(EditDefaultsOnly, Category = "GeoSummonedDummy")
	FGeoEnemySetup AllySetup;
};
