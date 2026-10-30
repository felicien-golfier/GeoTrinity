// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

void ULateralWavePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);
}

float ULateralWavePattern::GetHazardDuration() const
{
	return (TravelDistance + BandWidth) / Speed;
}

FVector2D ULateralWavePattern::GetForward() const
{
	return FVector2D(FRotator(0.f, TravelYaw, 0.f).Vector());
}

TArray<FSafeSpot> ULateralWavePattern::GetSafeSpots() const
{
	FVector2D const Forward = GetForward();
	FVector2D const Right(-Forward.Y, Forward.X);
	TArray<FVector2D> FixedLocations;
	if (FLateralWavePatternData const* const WaveData = StoredPatternData.GetPtr<FLateralWavePatternData>())
	{
		for (FVector2D const& Offset : WaveData->SafeSpotOffsets)
		{
			FixedLocations.Add(StoredPayload.Origin + Forward * Offset.X + Right * Offset.Y);
		}
	}

	TArray<ESafeSpotSource> Sources = SafeSpotSources;
	if (!GeoLib::IsServer(GetWorld()))
	{
		Sources.Remove(ESafeSpotSource::Adds);
	}

	return GatherSafeSpots(this, StoredPayload, Sources, ESafeSpotShadow::Lane, 0.f, SpotRadius, FixedLocations);
}

bool ULateralWavePattern::IsInHazard(AActor const* /*Target*/, FVector2D const Location, float const SpentTime) const
{
	FVector2D const Forward = GetForward();
	FVector2D const Right(-Forward.Y, Forward.X);
	FVector2D const FromOrigin = Location - StoredPayload.Origin;
	float const Along = FromOrigin | Forward;
	float const Across = FromOrigin | Right;
	float const Front = Speed * SpentTime;

	return Along >= 0.f && Along <= TravelDistance && Along <= Front && Along >= Front - BandWidth
		&& FMath::Abs(Across) <= HalfWidth
		&& !FSafeSpot::AnyShields(GetSafeSpots(), Location, StoredPayload.Origin, Forward);
}

void ULateralWavePattern::MoveBeamVfx(float const SpentTime)
{
	if (IsValid(BeamVisual))
	{
		FVector2D const Forward = GetForward();
		FVector2D const Right(-Forward.Y, Forward.X);
		FVector2D BeamStart = StoredPayload.Origin;
		FVector2D BeamDirection = Forward;
		float BeamLength = TravelDistance;
		float ShapeHalfWidth = HalfWidth;
		if (SpentTime >= 0.f)
		{
			float const BandEnd = FMath::Clamp(Speed * SpentTime, 0.f, TravelDistance);
			float const BandStart = FMath::Clamp(Speed * SpentTime - BandWidth, 0.f, TravelDistance);
			BeamStart = StoredPayload.Origin - Right * HalfWidth + Forward * ((BandStart + BandEnd) * 0.5f);
			BeamDirection = Right;
			BeamLength = HalfWidth * 2.f;
			ShapeHalfWidth = (BandEnd - BandStart) * 0.5f;
		}

		BeamVisual->SetWorldLocationAndRotation(FVector(BeamStart, ArbitraryCharacterZ),
												FVector(BeamDirection, 0.f).Rotation());
		BeamVisual->Shape.Length = BeamLength;
		BeamVisual->Shape.HalfWidth = ShapeHalfWidth;
		BeamVisual->bSafeSpotCut = true;
		FSafeSpotMask::Set(this, MaskMaterialParameterCollection, GetSafeSpots(), Forward);
	}
}

void ULateralWavePattern::EndPattern(bool const bForceStop)
{
	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);

	Super::EndPattern(bForceStop);
}
