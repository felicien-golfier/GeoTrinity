// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** The loops of the Cat's face, the Tutorial Board's: each is a pure function of the time in seconds. */
namespace GeoCatFaceMotion
{
	/** Vertical scale of the eyes: 1 open, squinting to .55 around 44-56 % of the cycle, blinking to .08 at 94 %. */
	GEOTRINITYUI_API float GetEyeScale(double Seconds);

	/** Angle in degrees of an ear's flick, 0 at rest, over its cycle shifted by EarIndex. */
	GEOTRINITYUI_API float GetEarAngle(double Seconds, int32 EarIndex);

	/** Vertical scale of the open mouth, going back and forth between .35 and 1. */
	GEOTRINITYUI_API float GetMouthScale(double Seconds);
} // namespace GeoCatFaceMotion
