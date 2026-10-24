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

void FSafeSpotMask::Set(UObject* WorldContext, UMaterialParameterCollection* Collection, TArray<FSafeSpot> const& Spots,
						FVector2D const LaneDirection)
{
	if (!GeoLib::IsDedicatedServer(WorldContext) && Collection)
	{
		TArray<FLinearColor> const SlotValues = GetSlotValues(Spots);
		for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(
				WorldContext, Collection, GeoMaterialParams::GetPillarPositionName(SlotIndex), SlotValues[SlotIndex]);
		}

		bool const bAnySpot = Spots.Num() > 0;
		UKismetMaterialLibrary::SetScalarParameterValue(WorldContext, Collection, GeoMaterialParams::PillarRadius,
														bAnySpot ? Spots[0].Radius : 0.f);
		UKismetMaterialLibrary::SetScalarParameterValue(WorldContext, Collection, GeoMaterialParams::SafeSpotShadowMode,
														bAnySpot ? static_cast<float>(Spots[0].Shadow)
																 : static_cast<float>(ESafeSpotShadow::Behind));
		UKismetMaterialLibrary::SetVectorParameterValue(WorldContext, Collection,
														GeoMaterialParams::SafeSpotLaneDirection,
														FLinearColor(LaneDirection.X, LaneDirection.Y, 0.f, 0.f));
	}
}

void FSafeSpotMask::Clear(UObject* WorldContext, UMaterialParameterCollection* Collection)
{
	Set(WorldContext, Collection, {}, FVector2D::ZeroVector);
}
