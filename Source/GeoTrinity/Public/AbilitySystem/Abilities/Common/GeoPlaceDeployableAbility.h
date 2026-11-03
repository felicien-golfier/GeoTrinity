// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoPlaceDeployableAbility.generated.h"

/**
 * Server-only ability that places a deployable on each location of its Target, with no projectile and no charge: an
 * enemy's zone. The caster is the deployable's instigator, so the deployable dies with it. No gem adjustment.
 */
UCLASS()
class GEOTRINITY_API UGeoPlaceDeployableAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoPlaceDeployableAbility();

protected:
	/** Server. Places one deployable per target location, then ends the ability. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|PlaceDeployable")
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

	/** The deployable's size, growth, colour and life drain as authored. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|PlaceDeployable")
	FDeployableDataParams Params;

	/** Replaces Params.Size, read at the ability's level so a difficulty column scales it; 0 keeps Params.Size. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|PlaceDeployable")
	FScalableFloat Size;
};
