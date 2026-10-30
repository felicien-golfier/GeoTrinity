// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/ZoneLayer/GeoZoneVisualComponent.h"

#include "Engine/World.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneLayerSubsystem.h"

UGeoZoneVisualComponent::UGeoZoneVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGeoZoneVisualComponent::OnRegister()
{
	Super::OnRegister();
	SetZoneVisible(bZoneVisible);
}

void UGeoZoneVisualComponent::OnUnregister()
{
	if (UGeoZoneLayerSubsystem* const Subsystem = UWorld::GetSubsystem<UGeoZoneLayerSubsystem>(GetWorld()))
	{
		Subsystem->Unregister(this);
	}

	Super::OnUnregister();
}

void UGeoZoneVisualComponent::SetZoneVisible(bool const bShow)
{
	bZoneVisible = bShow;
	// No subsystem on a dedicated server or in a preview world: nothing draws there.
	if (UGeoZoneLayerSubsystem* const Subsystem = UWorld::GetSubsystem<UGeoZoneLayerSubsystem>(GetWorld()))
	{
		if (bZoneVisible && IsRegistered())
		{
			Subsystem->Register(this);
		}
		else
		{
			Subsystem->Unregister(this);
		}
	}
}

void UGeoZoneVisualComponent::SetTimedFill(float const StartServerTime, float const Duration)
{
	TimedFillStartServerTime = StartServerTime;
	TimedFillDuration = Duration;
}

float UGeoZoneVisualComponent::GetFill() const
{
	if (TimedFillDuration > 0.f)
	{
		return FMath::Clamp((GeoLib::GetServerTime(this, true) - TimedFillStartServerTime) / TimedFillDuration, 0.f,
							1.f);
	}

	return Fill;
}
