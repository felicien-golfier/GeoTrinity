// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/Component/GeoFXComponent.h"
#include "CoreMinimal.h"

#include "GeoDeployableFXComponent.generated.h"

/**
 * Plays AGeoDeployableBase::FXMap's moments, resolving them against the *deployer* and not the deployable: a mine
 * carries no attributes of its own, and one dropped by the local player has to sound like their own.
 *
 * The owner fires the moments itself (AGeoDeployableBase::PlayMoment), since a deployable's life cycle — spawn, blink,
 * recall, explode, expire — is the actor's and not this component's.
 */
UCLASS(ClassGroup = "GeoTrinity")
class GEOTRINITY_API UGeoDeployableFXComponent : public UGeoFXComponent
{
	GENERATED_BODY()

protected:
	/** Who deployed it, off the owner's FDeployableData. */
	virtual AActor* GetFXInstigator() const override;

	/** The level it was deployed at, off the same data. */
	virtual int32 GetAbilityLevel() const override;
};
