// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "CoreMinimal.h"

#include "GeoSplitPassiveAbility.generated.h"

class AGeoDeployableBase;

/**
 * Passive of the Split Core: a deployable that lands from a throw places a copy of itself, starting at a fraction of
 * its health, at a random spot around it, at most once per its Cooldown GE. The copy carries the throw's Leverage. Only
 * a throw can split, so a copy never splits again.
 */
UCLASS()
class GEOTRINITY_API UGeoSplitPassiveAbility : public UGeoCorePassiveAbility
{
	GENERATED_BODY()

protected:
	/** Server: listens to the holder's OnDeployableLanded. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) override;
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) override;

private:
	/** Places a copy of Deployable, which just landed, when TryTrigger passes. */
	void SplitDeployable(AGeoDeployableBase& Deployable);

	/** Health the copy starts with, as a share of its max health; the max is the deployable's own. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Split",
			  meta = (ClampMin = "0.01", ClampMax = "1"))
	float StartHealthRatio = 0.5f;

	/** The copy lands this far from the deployable at least, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Split", meta = (ClampMin = "0"))
	float MinCopyDistance = 150.f;

	/** The copy lands this far from the deployable at most, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Split", meta = (ClampMin = "0"))
	float MaxCopyDistance = 300.f;
};
