// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/DevastatingWavePattern.h"

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotSource.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

static TAutoConsoleVariable
	CVarDrawDevastatingWave(TEXT("Geo.DrawDevastatingWave"), false,
							TEXT("When true, draws the devastating wave's front / inner radius and pillar safe zones"));

UDevastatingWavePattern::UDevastatingWavePattern()
{
	HazardColor.Color = EGeoColor::LethalDamage;
}

void UDevastatingWavePattern::InitPattern(FAbilityPayload const& Payload,
										  TInstancedStruct<FPatternData> const& PatternData)
{
	ClearData();
	Super::InitPattern(Payload, PatternData);
	if (!ensureMsgf(IsValid(StoredPayload.SourceOwner), TEXT("%hs: StoredPayload.SourceOwner is null"), __FUNCTION__))
	{
		return;
	}

	StoredPayload.SourceAvatar->SetActorLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));
}

void UDevastatingWavePattern::AddPillarSpots(float const MaxDistance)
{
	bool bAdded = false;
	for (FSafeSpot const& Spot : GatherSafeSpots(this, StoredPayload, {ESafeSpotSource::Pillars},
												 ESafeSpotShadow::Behind, MaxDistance, 0.f, {}))
	{
		if (!PillarSpots.ContainsByPredicate(
				[&Spot](FSafeSpot const& Known)
				{
					return Known.Source == Spot.Source;
				}))
		{
			PillarSpots.Add(Spot);
			bAdded = true;
		}
	}

	if (bAdded)
	{
		SetSafeSpots(PillarSpots, FVector2D::ZeroVector, StoredPayload.Origin);
	}
}

void UDevastatingWavePattern::ClearData()
{
	PillarSpots.Empty();
	ClearSafeSpots();
}

void UDevastatingWavePattern::StartPattern()
{
	ClearData();
	Super::StartPattern();
}

FGameplayCueParameters UDevastatingWavePattern::FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Cue, Payload);
	CueParams.RawMagnitude = MaxRadius;
	float const LifeTime = MaxRadius / ExpansionSpeed;
	CueParams.Normal = FVector(LifeTime, TravelTime, 1.f - (LifeTime - TravelTime) / LifeTime);
	return CueParams;
}

void UDevastatingWavePattern::TickPattern(float const ServerTime, float const SpentTime)
{
	Super::TickPattern(ServerTime, SpentTime);

	float const CurrentRadius = FMath::Min(ExpansionSpeed * SpentTime, MaxRadius);
	if (CurrentRadius > 0.f)
	{
		AddPillarSpots(CurrentRadius);
		if (CVarDrawDevastatingWave.GetValueOnGameThread())
		{
			DrawDebugWave(CurrentRadius);
		}
	}
}

void UDevastatingWavePattern::MoveHazardVisual(float const SpentTime)
{
	if (IsValid(HazardVisual))
	{
		HazardVisual->SetWorldLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));
		if (SpentTime < 0.f)
		{
			AddPillarSpots(0.f);
			HazardVisual->Shape.Shape = EGeoZoneShape::Circle;
			HazardVisual->Shape.Radius = MaxRadius;
		}
		else
		{
			float const FrontRadius = FMath::Min(ExpansionSpeed * SpentTime, MaxRadius);
			HazardVisual->Shape.Shape = EGeoZoneShape::Ring;
			HazardVisual->Shape.Radius = FrontRadius;
			HazardVisual->Shape.InnerRadius = FMath::Max(0.f, FrontRadius - AnnulusWidth);
		}
	}
}

float UDevastatingWavePattern::GetHazardDuration() const
{
	return MaxRadius / ExpansionSpeed;
}

bool UDevastatingWavePattern::IsInHazard(AActor const* /*Target*/, FVector2D const Location,
										 float const SpentTime) const
{
	float const WaveRadius = FMath::Min(ExpansionSpeed * SpentTime, MaxRadius);
	float const FrontOffset = WaveRadius - FVector2D::Distance(StoredPayload.Origin, Location);
	return FrontOffset >= 0.f && FrontOffset <= AnnulusWidth
		&& !FSafeSpot::AnyShields(PillarSpots, Location, StoredPayload.Origin, FVector2D::ZeroVector);
}

void UDevastatingWavePattern::DrawDebugWave(float CurrentRadius) const
{
	FVector const Center(StoredPayload.Origin, ArbitraryCharacterZ);
	DrawDebugCircle(GetWorld(), Center, CurrentRadius, 64, FColor::Red, false, 0.f, 0, 3.f, FVector::XAxisVector,
					FVector::YAxisVector, false);
	DrawDebugCircle(GetWorld(), Center, FMath::Max(0.f, CurrentRadius - AnnulusWidth), 64, FColor::Green, false, 0.f, 0,
					3.f, FVector::XAxisVector, FVector::YAxisVector, false);

	int i = 0;
	for (FSafeSpot const& PillarSpot : PillarSpots)
	{
		FVector2D const PillarLocation(PillarSpot.Location);
		float const PillarRadius = PillarSpot.Radius;
		FVector2D const CenterToPillar(PillarLocation - StoredPayload.Origin);
		float const PillarDistance = CenterToPillar.Size();
		FVector2D const PillarDir(CenterToPillar / PillarDistance);
		FVector2D const PillarPerp(-PillarDir.Y, PillarDir.X);

		FVector2D const LeftTangent(PillarLocation + PillarPerp * PillarRadius);
		FVector2D const RightTangent(PillarLocation - PillarPerp * PillarRadius);

		float const WaveOffset = FMath::Max(0.f, CurrentRadius - PillarDistance);
		FVector2D const LeftEnd(LeftTangent + PillarDir * WaveOffset);
		FVector2D const RightEnd(RightTangent + PillarDir * WaveOffset);

		FColor const Color = ColorPalette[i++ % UE_ARRAY_COUNT(ColorPalette)];
		DrawDebugLine(GetWorld(), FVector(LeftTangent, ArbitraryCharacterZ), FVector(LeftEnd, ArbitraryCharacterZ),
					  Color, false, 0.f, 0, 3.f);
		DrawDebugLine(GetWorld(), FVector(RightTangent, ArbitraryCharacterZ), FVector(RightEnd, ArbitraryCharacterZ),
					  Color, false, 0.f, 0, 3.f);
	}
}

void UDevastatingWavePattern::EndPattern(bool bForceStop)
{
	if (!UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		Super::EndPattern(bForceStop);
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.SourceOwner);
	if (!ensureMsgf(SourceASC, TEXT("%hs: Owner has no ASC on wave end"), __FUNCTION__))
	{
		Super::EndPattern(bForceStop);
		return;
	}

	if (!bForceStop)
	{
		for (FSafeSpot const& PillarSpot : PillarSpots)
		{
			if (!PillarSpot.Source.IsValid())
			{
				continue;
			}
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(PillarSpot.Source.Get());
			if (!ensureMsgf(TargetASC, TEXT("%hs: alive pillar has no ASC"), __FUNCTION__))
			{
				continue;
			}
			FLethalEffectData LethalEffectData;
			UGeoAbilitySystemLibrary::ApplySingleEffectData(LethalEffectData, SourceASC, TargetASC,
															StoredPayload.AbilityLevel, StoredPayload.Seed,
															StoredPayload.AbilityTag);
			UGeoAbilitySystemLibrary::NotifyAbilityHit(StoredPayload, PillarSpot.Source.Get());
		}
	}

	ClearData();

	Super::EndPattern(bForceStop);
}
