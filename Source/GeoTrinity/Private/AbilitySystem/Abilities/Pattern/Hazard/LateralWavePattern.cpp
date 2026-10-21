// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "NiagaraComponent.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "Tool/Visual/GeoNiagaraParams.h"

void ULateralWavePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);

	Super::InitPattern(Payload, PatternData);
}

void ULateralWavePattern::TickDuringInit(float const SpentTime)
{
	Super::TickDuringInit(SpentTime);
	RefreshMask();
}

void ULateralWavePattern::TickPattern(float const ServerTime, float const SpentTime)
{
	Super::TickPattern(ServerTime, SpentTime);
	RefreshMask();
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
	for (FVector2D const& Offset : FixedSpotOffsets)
	{
		FixedLocations.Add(StoredPayload.Origin + Forward * Offset.X + Right * Offset.Y);
	}

	return GatherSafeSpots(this, StoredPayload, SafeSpotSources, ESafeSpotShadow::Lane, 0.f, SpotRadius,
						   FixedLocations);
}

void ULateralWavePattern::RefreshMask()
{
	FSafeSpotMask::Set(this, MaskMaterialParameterCollection, GetSafeSpots());
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
	if (IsValid(BeamVfxComponent))
	{
		FVector2D const Forward = GetForward();
		FVector2D const Right(-Forward.Y, Forward.X);
		FVector2D BeamStart = StoredPayload.Origin;
		FVector2D BeamDirection = Forward;
		float BeamLength = TravelDistance;
		float BeamWidth = HalfWidth * 2.f;

		if (SpentTime >= 0.f)
		{
			float const BandEnd = FMath::Clamp(Speed * SpentTime, 0.f, TravelDistance);
			float const BandStart = FMath::Clamp(Speed * SpentTime - BandWidth, 0.f, TravelDistance);
			BeamStart = StoredPayload.Origin - Right * HalfWidth + Forward * ((BandStart + BandEnd) * 0.5f);
			BeamDirection = Right;
			BeamLength = HalfWidth * 2.f;
			BeamWidth = BandEnd - BandStart;
		}

		BeamVfxComponent->SetWorldLocationAndRotation(FVector(BeamStart, ArbitraryCharacterZ),
													  FVector(BeamDirection, 0.f).Rotation());
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::BeamLength, BeamLength);
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::BeamWidth, BeamWidth);
	}
}

void ULateralWavePattern::EndPattern(bool const bForceStop)
{
	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);

	Super::EndPattern(bForceStop);
}
