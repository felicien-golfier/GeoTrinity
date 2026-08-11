// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Interface/GeoChargeGaugeWidgetInterface.h"

#include "GeoChargeBeamGaugeWidgetInterface.generated.h"

UINTERFACE()
class UGeoChargeBeamGaugeWidgetInterface : public UGeoChargeGaugeWidgetInterface
{
	GENERATED_BODY()
};

/**
 * The charge-beam gauge on top of the shared charge-gauge seam: it is the only one with a sweet-spot window to mark.
 */
class IGeoChargeBeamGaugeWidgetInterface : public IGeoChargeGaugeWidgetInterface
{
	GENERATED_BODY()

public:
	/** Sets the sweet-spot window boundaries, marking the layout dirty for the next tick. */
	virtual void SetSweetSpotRatios(float MinRatio, float MaxRatio) = 0;
};
