// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/BeamPattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/ZoneLayer/GeoZoneVisualComponent.h"

static TAutoConsoleVariable CVarDrawBeamBorder(TEXT("Geo.DrawBeamBorder"), false,
											   TEXT("When true, draws the beam pattern's hit-scan rectangle borders"));

float UBeamPattern::GetBeamYaw(float const SpentTime, float const BossServerTime) const
{
	if (FollowBossOrientation && IsValid(StoredPayload.SourceAvatar))
	{
		return GeoLib::GetPoseAt(StoredPayload.SourceAvatar, BossServerTime).Yaw;
	}

	float const SweptFraction = FMath::Clamp(SpentTime / BeamDuration, 0.f, 1.f);
	float const SweepSign = StoredPayload.Seed % 2 == 0 ? 1.f : -1.f;
	return GetTargetYaw() + SweepSign * SweepAngle * (1.f - 2.f * SweptFraction);
}

FVector UBeamPattern::GetBeamOrigin(float const BossServerTime) const
{
	if (FollowBossLocation && IsValid(StoredPayload.SourceAvatar))
	{
		return GeoLib::GetPoseAt(StoredPayload.SourceAvatar, BossServerTime).Location;
	}

	return FVector(StoredPayload.Origin, ArbitraryCharacterZ);
}

void UBeamPattern::MoveHazardVisual(float const SpentTime)
{
	if (IsValid(HazardVisual))
	{
		float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
		HazardVisual->Shape.Shape = EGeoZoneShape::Beam;
		HazardVisual->Shape.Length = BeamRange;
		HazardVisual->Shape.HalfWidth = BeamHalfWidth;
		HazardVisual->SetWorldLocationAndRotation(GetBeamOrigin(ServerTime),
												  FRotator(0.f, GetBeamYaw(SpentTime, ServerTime), 0.f));
	}
}

void UBeamPattern::TickPattern(float const ServerTime, float const SpentTime)
{
	Super::TickPattern(ServerTime, SpentTime);

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

FGameplayCueParameters UBeamPattern::FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Cue, Payload);
	CueParams.RawMagnitude = BeamRange;
	return CueParams;
}
