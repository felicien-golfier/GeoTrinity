// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GeoDeployGaugeWidgetInterface.generated.h"

class UGeoGameplayAbility;

UINTERFACE()
class UGeoDeployGaugeWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by the UI module's deploy-charge gauge widget so gameplay (APlayableCharacter) can set the driving
 * ability without naming the concrete UI type.
 */
class IGeoDeployGaugeWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sets the deploy ability whose charge ratio drives the gauge fill. */
	virtual void SetDeployAbility(UGeoGameplayAbility* Ability) = 0;
};
