// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "CoreMinimal.h"

#include "GeoSplitPassiveAbility.generated.h"

class AGeoDeployableBase;

/**
 * Passive of the Split Core: a deployable that lands from a throw places a copy of itself, with a fraction of its
 * health, at a random spot around it, at most once per CooldownSeconds. Only a throw can split, so a copy never splits
 * again.
 */
UCLASS()
class GEOTRINITY_API UGeoSplitPassiveAbility : public UGeoCorePassiveAbility
{
	GENERATED_BODY()

public:
	UGeoSplitPassiveAbility();

protected:
	/** Server: listens to the holder's OnDeployableLanded. */
	virtual void BindEvent(UGeoAbilitySystemComponent& HolderASC) override;
	virtual void UnbindEvent(UGeoAbilitySystemComponent& HolderASC) override;

private:
	/** Places a copy of Deployable, which just landed, when TryTrigger passes. */
	void SplitDeployable(AGeoDeployableBase& Deployable);

	/** Health of the copy, as a fraction of the deployable's. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Split", meta = (ClampMin = "0", ClampMax = "1"))
	float CopyHealthFraction = 0.5f;

	/** The copy lands this far from the deployable at least, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Split", meta = (ClampMin = "0"))
	float MinCopyDistance = 150.f;

	/** The copy lands this far from the deployable at most, in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Split", meta = (ClampMin = "0"))
	float MaxCopyDistance = 300.f;
};
