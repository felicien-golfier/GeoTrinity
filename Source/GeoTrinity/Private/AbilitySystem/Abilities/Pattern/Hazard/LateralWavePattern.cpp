// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/Arena/GeoArenaVolume.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

float ULateralWavePattern::RollTravelYaw(int32 const Seed)
{
	FRandomStream Stream(Seed);
	return 90.f * Stream.RandRange(0, 3);
}

void ULateralWavePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	FiringOrigin = Payload.Origin;
	FiringYaw = TravelYaw;
	FiringTravelDistance = TravelDistance;
	FiringHalfWidth = HalfWidth;
	FLateralWavePatternData const* const WaveData = PatternData.GetPtr<FLateralWavePatternData>();
	if (WaveData && WaveData->bRandomDirection)
	{
		RollDirection(Payload);
	}

	Super::InitPattern(Payload, PatternData);

	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);
}

void ULateralWavePattern::RollDirection(FAbilityPayload const& Payload)
{
	FiringYaw = RollTravelYaw(Payload.Seed);
	AGeoArena const* const Arena = AGeoArena::GetArenaOfBoss(Payload.SourceAvatar);
	if (ensureMsgf(Arena, TEXT("%hs: a rolled wave needs the caster's arena to fit to"), __FUNCTION__))
	{
		FBox2D const Bounds = AGeoArenaVolume::GetArenaBounds(this, Arena->ArenaTag);
		if (ensureMsgf(Bounds.bIsValid, TEXT("%hs: no arena volume carries %s"), __FUNCTION__,
					   *Arena->ArenaTag.ToString()))
		{
			FVector2D const Forward = GetForward();
			FVector2D const HalfSize = Bounds.GetExtent();
			float const HalfAlong = FMath::Abs(Forward.X) * HalfSize.X + FMath::Abs(Forward.Y) * HalfSize.Y;
			FiringOrigin = Bounds.GetCenter() - Forward * HalfAlong;
			FiringTravelDistance = HalfAlong * 2.f;
			FiringHalfWidth = FMath::Abs(Forward.Y) * HalfSize.X + FMath::Abs(Forward.X) * HalfSize.Y;
		}
	}
}

float ULateralWavePattern::GetHazardDuration() const
{
	return (FiringTravelDistance + BandWidth) / Speed;
}

FVector2D ULateralWavePattern::GetForward() const
{
	return FVector2D(FRotator(0.f, FiringYaw, 0.f).Vector());
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
			FixedLocations.Add(FiringOrigin + Forward * Offset.X + Right * Offset.Y);
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
	FVector2D const FromOrigin = Location - FiringOrigin;
	float const Along = FromOrigin | Forward;
	float const Across = FromOrigin | Right;
	float const Front = Speed * SpentTime;

	return Along >= 0.f && Along <= FiringTravelDistance && Along <= Front && Along >= Front - BandWidth
		&& FMath::Abs(Across) <= FiringHalfWidth
		&& !FSafeSpot::AnyShields(GetSafeSpots(), Location, FiringOrigin, Forward);
}

void ULateralWavePattern::MoveBeamVfx(float const SpentTime)
{
	if (IsValid(BeamVisual))
	{
		FVector2D const Forward = GetForward();
		FVector2D const Right(-Forward.Y, Forward.X);
		FVector2D BeamStart = FiringOrigin;
		FVector2D BeamDirection = Forward;
		float BeamLength = FiringTravelDistance;
		float ShapeHalfWidth = FiringHalfWidth;
		if (SpentTime >= 0.f)
		{
			float const BandEnd = FMath::Clamp(Speed * SpentTime, 0.f, FiringTravelDistance);
			float const BandStart = FMath::Clamp(Speed * SpentTime - BandWidth, 0.f, FiringTravelDistance);
			BeamStart = FiringOrigin - Right * FiringHalfWidth + Forward * ((BandStart + BandEnd) * 0.5f);
			BeamDirection = Right;
			BeamLength = FiringHalfWidth * 2.f;
			ShapeHalfWidth = (BandEnd - BandStart) * 0.5f;
		}
		else if (StartDelay > 0.f)
		{
			BeamLength *= FMath::Clamp(1.f + SpentTime / StartDelay, 0.f, 1.f);
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
