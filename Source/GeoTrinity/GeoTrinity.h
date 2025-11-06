// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Project-wide log category for GeoTrinity
DECLARE_LOG_CATEGORY_EXTERN(LogGeoTrinity, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGeoASC, Log, All);

// Used to limit the number of snapshots stored for rollback/replay
inline constexpr int MaxBufferInputs = 30;
