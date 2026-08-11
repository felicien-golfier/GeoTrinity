// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GeoChargeGaugeWidgetInterface.generated.h"

class UGeoGameplayAbility;

UINTERFACE()
class UGeoChargeGaugeWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Shared seam for every world-space charge gauge (deploy charge, charge beam), so APlayableCharacter shows and hides
 * them all through one code path without naming the concrete UI types.
 */
class IGeoChargeGaugeWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sets the ability whose charge ratio drives the gauge fill; null detaches the gauge. */
	virtual void SetChargeAbility(UGeoGameplayAbility* Ability) = 0;
	/** Syncs the bar fill to the current ability's charge ratio. Safe to call outside of tick. */
	virtual void UpdateVisualChargeRatio() const = 0;
};
