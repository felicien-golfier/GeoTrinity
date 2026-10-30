// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/BeamPattern.h"

#include "AbilitySystem/Abilities/Boss/GeoSweepBeamAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

static TAutoConsoleVariable CVarDrawBeamBorder(TEXT("Geo.DrawBeamBorder"), false,
											   TEXT("When true, draws the beam pattern's hit-scan rectangle borders"));

UBeamPattern::UBeamPattern()
{
	bHasHazard = true;
}

void UBeamPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (UGeoSweepBeamAbility const* SweepBeamAbility = GeoASLib::GetAbilityCDO<UGeoSweepBeamAbility>(AbilityTag))
	{
		SweepAngle = SweepBeamAbility->GetSweepAngle();
	}

	if (!GeoLib::IsDedicatedServer(GetWorld()))
	{
		BeamVisual = NewObject<UGeoZoneVisualComponent>(&Owner);
		BeamVisual->SetupAttachment(Owner.GetRootComponent());
		// Placed in world space by MoveBeamVfx, whatever the owner does.
		BeamVisual->SetAbsolute(true, true, true);
		BeamVisual->Shape.Shape = EGeoZoneShape::Beam;
		BeamVisual->ColorMask = GeoZoneLayer::MakeColorMask(BeamColor, SecondaryBeamColors);
		BeamVisual->SetZoneVisible(false);
		BeamVisual->RegisterComponent();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UBeamPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

	if (IsValid(BeamVisual))
	{
		BeamVisual->Shape.Length = BeamRange;
		BeamVisual->Shape.HalfWidth = BeamHalfWidth;
		BeamVisual->Layer = EGeoZoneLayer::Telegraph;
		BeamVisual->SetTimedFill(StoredPayload.ServerSpawnTime, StartDelay);
		BeamVisual->SetZoneVisible(true);
	}
}

float UBeamPattern::GetBeamYaw(float const SpentTime, float const BossServerTime) const
{
	if (FollowBossOrientation && IsValid(StoredPayload.SourceAvatar))
	{
		return GeoLib::GetPoseAt(StoredPayload.SourceAvatar, BossServerTime).Yaw;
	}

	float const SweptFraction = FMath::Clamp(SpentTime / BeamDuration, 0.f, 1.f);
	float const SweepSign = StoredPayload.Seed % 2 == 0 ? 1.f : -1.f;
	return StoredPayload.Yaw - SweepSign * (2.f * SweepAngle * SweptFraction);
}

FVector UBeamPattern::GetBeamOrigin(float const BossServerTime) const
{
	if (FollowBossLocation && IsValid(StoredPayload.SourceAvatar))
	{
		return GeoLib::GetPoseAt(StoredPayload.SourceAvatar, BossServerTime).Location;
	}

	return FVector(StoredPayload.Origin, ArbitraryCharacterZ);
}

void UBeamPattern::MoveBeamVfx(float const SpentTime)
{
	if (IsValid(BeamVisual))
	{
		float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
		BeamVisual->SetWorldLocationAndRotation(GetBeamOrigin(ServerTime),
												FRotator(0.f, GetBeamYaw(SpentTime, ServerTime), 0.f));
	}
}

void UBeamPattern::TickDuringInit(float const SpentTime)
{
	MoveBeamVfx(SpentTime);
}

void UBeamPattern::StartPattern()
{
	if (IsValid(BeamVisual))
	{
		BeamVisual->Layer = EGeoZoneLayer::Ground;
		BeamVisual->SetTimedFill(StoredPayload.ServerSpawnTime + StartDelay, GetHazardDuration());
	}

	Super::StartPattern();
}

void UBeamPattern::TickPattern(float const ServerTime, float const SpentTime)
{
	MoveBeamVfx(SpentTime);

	if (GeoLib::IsServer(GetWorld()) && CVarDrawBeamBorder.GetValueOnGameThread())
	{
		FVector const Location = GetBeamOrigin(ServerTime);
		FVector const Forward = FRotator(0.f, GetBeamYaw(SpentTime, ServerTime), 0.f).Vector();
		FVector const Right = FVector::CrossProduct(FVector::UpVector, Forward);
		FVector const BeamStart = Location;
		FVector const BeamEnd = Location + Forward * BeamRange;
		DrawDebugLine(GetWorld(), BeamStart + Right * BeamHalfWidth, BeamEnd + Right * BeamHalfWidth, FColor::Red,
					  false, 0.f);
		DrawDebugLine(GetWorld(), BeamStart - Right * BeamHalfWidth, BeamEnd - Right * BeamHalfWidth, FColor::Red,
					  false, 0.f);
		DrawDebugLine(GetWorld(), BeamEnd - Right * BeamHalfWidth, BeamEnd + Right * BeamHalfWidth, FColor::Red, false,
					  0.f);
	}
}

float UBeamPattern::GetHazardDuration() const
{
	return BeamDuration;
}

bool UBeamPattern::IsInHazard(AActor const* Target, FVector2D const Location, float const SpentTime) const
{
	float const TargetServerTime =
		StoredPayload.ServerSpawnTime + StartDelay + SpentTime - GeoLib::GetReplicationDelay(Target);
	FVector2D const Forward(FRotator(0.f, GetBeamYaw(SpentTime, TargetServerTime), 0.f).Vector());
	return GeoASLib::IsInLine(Target, Location, FVector2D(GetBeamOrigin(TargetServerTime)), Forward, BeamRange,
							  BeamHalfWidth, OverlapMode, GeoASLib::GetTeamId(StoredPayload.SourceOwner));
}

void UBeamPattern::OnHazardEnd()
{
	Super::OnHazardEnd();
	if (IsValid(BeamVisual))
	{
		BeamVisual->SetZoneVisible(false);
	}
}

FGameplayCueParameters UBeamPattern::FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Cue, Payload);
	CueParams.RawMagnitude = BeamRange;
	return CueParams;
}

void UBeamPattern::EndPattern(bool const bForceStop)
{
	if (IsValid(BeamVisual))
	{
		BeamVisual->SetZoneVisible(false);
	}

	Super::EndPattern(bForceStop);
}
