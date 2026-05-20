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
ENUM_CLASS_FLAGS(ETeamAttitudeBitflag)

namespace TeamAttitudeMask
{
	constexpr uint8 None = 0;
	constexpr uint8 Neutral = static_cast<uint8>(ETeamAttitudeBitflag::Neutral);
	constexpr uint8 Hostile = static_cast<uint8>(ETeamAttitudeBitflag::Hostile);
	constexpr uint8 Friendly = static_cast<uint8>(ETeamAttitudeBitflag::Friendly);
	constexpr uint8 All = static_cast<uint8>(ETeamAttitudeBitflag::Neutral)
		| static_cast<uint8>(ETeamAttitudeBitflag::Friendly) | static_cast<uint8>(ETeamAttitudeBitflag::Hostile);
	constexpr uint8 HostileOrFriendly =
		static_cast<uint8>(ETeamAttitudeBitflag::Friendly) | static_cast<uint8>(ETeamAttitudeBitflag::Hostile);
	constexpr uint8 HostileOrNeutral =
		static_cast<uint8>(ETeamAttitudeBitflag::Hostile) | static_cast<uint8>(ETeamAttitudeBitflag::Neutral);
	constexpr uint8 FriendlyOrNeutral =
		static_cast<uint8>(ETeamAttitudeBitflag::Friendly) | static_cast<uint8>(ETeamAttitudeBitflag::Neutral);
} // namespace TeamAttitudeMask
