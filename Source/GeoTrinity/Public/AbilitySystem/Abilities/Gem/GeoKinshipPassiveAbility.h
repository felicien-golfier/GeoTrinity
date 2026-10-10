// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "CoreMinimal.h"

#include "GeoKinshipPassiveAbility.generated.h"

class AGeoDeployableBase;

/**
 * Passive of the Kinship Core: when a deployable of the holder is destroyed, at most once per CooldownSeconds, a random
 * living ally deploys their own where it stood. The ally owns it and their gems apply to it; it spends no charge.
 * Only a deployable whose health a hit took to zero counts, once its blink is over: not its life draining away, not a
 * recall.
 */
UCLASS()
class GEOTRINITY_API UGeoKinshipPassiveAbility : public UGeoCorePassiveAbility
{
	GENERATED_BODY()

public:
	UGeoKinshipPassiveAbility();

protected:
	/** Server: listens to the holder's OnDeployableDestroyedByDamage. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) override;
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) override;

private:
	/** Lets a random living ally deploy where Deployable stood, when TryTrigger passes. */
	void OnDeployableDestroyed(AGeoDeployableBase& Deployable);
};
