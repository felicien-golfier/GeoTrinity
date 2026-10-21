// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"

#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "Tool/Visual/GeoNiagaraParams.h"

TArray<FLinearColor> FSafeSpotMask::GetSlotValues(TArray<FSafeSpot> const& Spots)
{
	if (Spots.Num() > MaxSlots)
	{
		UE_LOG(LogPattern, Warning, TEXT("FSafeSpotMask: %d safe spots for %d mask slots, skipping the rest"),
			   Spots.Num(), MaxSlots);
	}

	// Matches the "unused slot" default of MPC_MaskedArea's PillarPosWS_XX parameters.
	TArray<FLinearColor> SlotValues;
	SlotValues.Init(FLinearColor(-10000.f, -10000.f, -10000.f, 0.f), MaxSlots);
	for (int32 SlotIndex = 0; SlotIndex < FMath::Min(Spots.Num(), MaxSlots); ++SlotIndex)
	{
		SlotValues[SlotIndex] = FLinearColor(Spots[SlotIndex].Location.X, Spots[SlotIndex].Location.Y, 0.f);
	}

	return SlotValues;
}

void FSafeSpotMask::Set(UObject* WorldContext, UMaterialParameterCollection* Collection, TArray<FSafeSpot> const& Spots)
{
	if (!GeoLib::IsDedicatedServer(WorldContext) && Collection)
	{
		TArray<FLinearColor> const SlotValues = GetSlotValues(Spots);
		for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(
				WorldContext, Collection, GeoMaterialParams::GetPillarPositionName(SlotIndex), SlotValues[SlotIndex]);
		}

		bool const bFirstIsLane = Spots.Num() > 0 && Spots[0].Shadow == ESafeSpotShadow::Lane;
		UKismetMaterialLibrary::SetScalarParameterValue(WorldContext, Collection, GeoMaterialParams::PillarRadius,
														Spots.Num() > 0 ? Spots[0].Radius : 0.f);
		UKismetMaterialLibrary::SetScalarParameterValue(WorldContext, Collection, GeoMaterialParams::SafeSpotShadowLane,
														bFirstIsLane ? 1.f : 0.f);
	}
}

void FSafeSpotMask::Clear(UObject* WorldContext, UMaterialParameterCollection* Collection)
{
	Set(WorldContext, Collection, {});
}
