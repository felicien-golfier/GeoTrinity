// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Team.generated.h"

UENUM()
enum class ETeam : uint8
{
	Neutral,
	Player,
	Enemy
};

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETeamAttitudeBitflag : uint8
{
	Neutral = (1 << 0) UMETA(DisplayName = "Neutral"),
	Friendly = (1 << 1) UMETA(DisplayName = "Friendly"),
	Hostile = (1 << 2) UMETA(DisplayName = "Hostile")
};
