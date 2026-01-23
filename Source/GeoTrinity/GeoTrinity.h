// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Project-wide log category for GeoTrinity
DECLARE_LOG_CATEGORY_EXTERN(LogGeoTrinity, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGeoASC, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogPattern, Log, All);

#define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel1