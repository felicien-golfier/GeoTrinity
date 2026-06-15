// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GeoChargeBeamGaugeWidgetInterface.generated.h"

class UGeoGameplayAbility;

UINTERFACE()
class UGeoChargeBeamGaugeWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by the UI module's charge-beam gauge widget so gameplay (APlayableCharacter) can drive the gauge
 * without naming the concrete UI type.
 */
class IGeoChargeBeamGaugeWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sets the charging ability that drives the gauge fill. */
	virtual void SetChargeBeamAbility(UGeoGameplayAbility* Ability) = 0;
	/** Sets the sweet-spot window boundaries, marking the layout dirty for the next tick. */
	virtual void SetSweetSpotRatios(float MinRatio, float MaxRatio) = 0;
	/** Syncs the bar fill/color to the current ability charge ratio. Safe to call outside of tick. */
	virtual void UpdateVisualChargeRatio() const = 0;
};
