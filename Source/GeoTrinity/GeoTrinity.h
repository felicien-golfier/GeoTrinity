// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Project-wide log categories for GeoTrinity. Exported with the module API so other modules (e.g. GeoTrinityUI) that
// link against this module can use them across the DLL boundary.
GEOTRINITY_API DECLARE_LOG_CATEGORY_EXTERN(LogGeoTrinity, Log, All);
GEOTRINITY_API DECLARE_LOG_CATEGORY_EXTERN(LogGeoASC, Log, All);
GEOTRINITY_API DECLARE_LOG_CATEGORY_EXTERN(LogPattern, Log, All);

#define ECC_GeoCharacter ECollisionChannel::ECC_GameTraceChannel1
#define ECC_MovementBlocker ECollisionChannel::ECC_GameTraceChannel2
#define ECC_GeoProjectile ECollisionChannel::ECC_GameTraceChannel3
