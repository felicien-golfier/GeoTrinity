// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GeoMenuGuideGoal.generated.h"

/** A path through the pause menu a guide leads the player along (GeoTrinityUI, HUD/Menu/Frame/CLAUDE.md). */
UENUM(BlueprintType)
enum class EGeoMenuGuideGoal : uint8
{
	None,
	/** Gems page, Loadout: slot a Chip into an open Chip socket. */
	EquipChip,
	/** Gems page, Forge: craft a Chip. */
	ForgeChip,
	/** No control to point at: dims the menu and refuses every click (the Cat is speaking, nothing to do yet). */
	Hold,
};
