// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/ZoneHazardPattern.h"

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

UZoneHazardPattern::UZoneHazardPattern()
{
	bHasHazard = true;
	HazardColor.Color = EGeoColor::Damage;
}

void UZoneHazardPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (!GeoLib::IsDedicatedServer(GetWorld()))
	{
		HazardVisual = NewObject<UGeoZoneVisualComponent>(&Owner);
		HazardVisual->SetupAttachment(Owner.GetRootComponent());
		// Placed in world space by MoveHazardVisual, whatever the owner does.
		HazardVisual->SetAbsolute(true, true, true);
		HazardVisual->ColorMask = GeoZoneLayer::MakeColorMask(HazardColor, SecondaryHazardColors);
		HazardVisual->SetZoneVisible(false);
		HazardVisual->RegisterComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UZoneHazardPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	ClearSafeSpots();
	Super::InitPattern(Payload, PatternData);

	if (IsValid(HazardVisual) && StartSectionTimerHandle.IsValid())
	{
		HazardVisual->Layer = EGeoZoneLayer::Telegraph;
		HazardVisual->FillMode = TelegraphFillMode;
		HazardVisual->SetTimedFill(StoredPayload.ServerSpawnTime, StartDelay);
		MoveHazardVisual(-StartDelay);
		HazardVisual->SetZoneVisible(true);
	}
}

void UZoneHazardPattern::StartPattern()
{
	if (IsValid(HazardVisual))
	{
		HazardVisual->Layer = EGeoZoneLayer::Ground;
		HazardVisual->FillMode = HazardFillMode;
		HazardVisual->SetTimedFill(StoredPayload.ServerSpawnTime + StartDelay, GetHazardDuration());
		MoveHazardVisual(0.f);
		HazardVisual->SetZoneVisible(true);
	}

	Super::StartPattern();
}

void UZoneHazardPattern::TickDuringInit(float const SpentTime)
{
	MoveHazardVisual(SpentTime);
}

void UZoneHazardPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	MoveHazardVisual(SpentTime);
}

void UZoneHazardPattern::MoveHazardVisual(float /*SpentTime*/)
{
}

void UZoneHazardPattern::SetSafeSpots(TArray<FSafeSpot> const& Spots, FVector2D const LaneDirection,
									  FVector2D const Origin)
{
	FSafeSpotMask::Set(this, MaskMaterialParameterCollection, Spots, LaneDirection, Origin);
	if (IsValid(HazardVisual))
	{
		HazardVisual->bSafeSpotCut = true;
	}
}

void UZoneHazardPattern::ClearSafeSpots()
{
	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);
}

void UZoneHazardPattern::OnHazardEnd()
{
	Super::OnHazardEnd();
	if (IsValid(HazardVisual))
	{
		HazardVisual->SetZoneVisible(false);
	}
}

void UZoneHazardPattern::EndPattern(bool const bForceStop)
{
	if (IsValid(HazardVisual))
	{
		HazardVisual->SetZoneVisible(false);
	}

	ClearSafeSpots();

	Super::EndPattern(bForceStop);
}
