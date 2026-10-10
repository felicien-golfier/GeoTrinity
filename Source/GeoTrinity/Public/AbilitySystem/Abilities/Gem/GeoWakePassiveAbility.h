// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "CoreMinimal.h"

#include "GeoWakePassiveAbility.generated.h"

/**
 * Passive of the Wake Core: a dash leaves the holder's deployable where it started, at most once per CooldownSeconds.
 * It spends no deploy charge and has the health of a deployable thrown without Leverage.
 */
UCLASS()
class GEOTRINITY_API UGeoWakePassiveAbility : public UGeoCorePassiveAbility
{
	GENERATED_BODY()

public:
	UGeoWakePassiveAbility();

protected:
	/** Server: listens to the holder's OnDashStarted. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) override;
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) override;

private:
	/** Spawns the holder's deployable at DashStart when TryTrigger passes. */
	void OnDashStarted(FVector const& DashStart);
};
