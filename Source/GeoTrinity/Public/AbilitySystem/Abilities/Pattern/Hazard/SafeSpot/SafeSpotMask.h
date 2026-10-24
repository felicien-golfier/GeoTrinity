// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "CoreMinimal.h"

class UMaterialParameterCollection;

/** Upload of safe spots to the mask MPC (MPC_MaskedArea), where the hazard's material cuts their shadows out. The MPC
 * is global: one live user at a time. */
struct GEOTRINITY_API FSafeSpotMask
{
	static int32 constexpr MaxSlots = 8;

	/** One value per slot: the (X, Y) of the first MaxSlots Spots, the rest logged and skipped, unused slots the
	 * sentinel the material ignores. */
	static TArray<FLinearColor> GetSlotValues(TArray<FSafeSpot> const& Spots);

	/** Writes Spots to Collection: their slots, the radius and the shadow mode of the first, and LaneDirection (unit
	 * XY, read by Lane only). Does nothing on a dedicated server or without a Collection. */
	static void Set(UObject* WorldContext, UMaterialParameterCollection* Collection, TArray<FSafeSpot> const& Spots,
					FVector2D LaneDirection);

	/** Frees every slot. */
	static void Clear(UObject* WorldContext, UMaterialParameterCollection* Collection);
};
