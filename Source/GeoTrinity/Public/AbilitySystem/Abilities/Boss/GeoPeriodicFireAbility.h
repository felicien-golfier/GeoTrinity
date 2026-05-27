// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"
#include "CoreMinimal.h"

#include "GeoPeriodicFireAbility.generated.h"

/**
 * Passive ability that fires projectiles at all players on a repeating timer.
 * Server-only: no client prediction. Targets are resolved each shot via EProjectileTarget::AllPlayers.
 * Auto-activates when the ASC is assigned (carries Ability.Type.Passive tag).
 */
UCLASS()
class GEOTRINITY_API UGeoPeriodicFireAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

public:
	UGeoPeriodicFireAbility();

protected:
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float FireIntervalMin = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float FireIntervalMax = 3.f;
};
