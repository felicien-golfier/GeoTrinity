// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/BeamPattern.h"

#include "AbilitySystem/Abilities/Boss/GeoSweepBeamAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

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

	UGameDataSettings const* const GDSettings = GetDefault<UGameDataSettings>();
	IndicatorSystem = GDSettings->GetLoadedDataAsset(GDSettings->RayIndicatorSystem);

	UNiagaraSystem* const InitialAsset = IndicatorSystem ? IndicatorSystem : BeamVfxSystem;
	if (InitialAsset && !GeoLib::IsDedicatedServer(GetWorld()))
	{
		BeamVfxComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, InitialAsset, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ false, /*bAutoActivate*/ false);
		ensureMsgf(BeamVfxComponent, TEXT("UBeamPattern: failed to spawn the beam VFX system"));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UBeamPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

	if (IsValid(BeamVfxComponent))
	{
		// Telegraphs where the beam will land during the windup (montage Start section), TickDuringInit keeping it
		// aimed; StartPattern swaps this to the real BeamVfxSystem once the beam actually goes live.
		GeoNiagaraParams::ApplySwappableAsset(BeamVfxComponent, {BeamVfxSystem, IndicatorSystem},
											  /*bWantIndicator*/ true);
		BeamVfxComponent->Activate(true);
		BeamVfxComponent->AdvanceSimulationByTime(FMath::Max(TravelTime, 0.f), GetWorld()->GetDeltaSeconds());
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::Lifetime, StartDelay);
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::BeamLength, BeamRange);
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::BeamWidth, BeamHalfWidth * 2.f);
		BeamVfxComponent->SetColorParameter(GeoNiagaraParams::Color, BeamColor.GetColor());
	}
}

float UBeamPattern::GetBeamYaw(float const SpentTime) const
{
	if (FollowBossOrientation && IsValid(StoredPayload.SourceAvatar))
	{
		return StoredPayload.SourceAvatar->GetActorRotation().Yaw;
	}

	float const SweptFraction = FMath::Clamp(SpentTime / BeamDuration, 0.f, 1.f);
	float const SweepSign = StoredPayload.Seed % 2 == 0 ? 1.f : -1.f;
	return StoredPayload.Yaw - SweepSign * (2.f * SweepAngle * SweptFraction);
}

FVector UBeamPattern::GetBeamOrigin() const
{
	if (FollowBossLocation && IsValid(StoredPayload.SourceAvatar))
	{
		return StoredPayload.SourceAvatar->GetActorLocation();
	}

	return FVector(StoredPayload.Origin, ArbitraryCharacterZ);
}

void UBeamPattern::MoveBeamVfx(float const SpentTime)
{
	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->SetWorldLocationAndRotation(GetBeamOrigin(), FRotator(0.f, GetBeamYaw(SpentTime), 0.f));
	}
}

void UBeamPattern::TickDuringInit(float const SpentTime)
{
	MoveBeamVfx(SpentTime);
}

void UBeamPattern::StartPattern()
{
	if (IsValid(BeamVfxComponent))
	{
		GeoNiagaraParams::ApplySwappableAsset(BeamVfxComponent, {BeamVfxSystem, IndicatorSystem},
											  /*bWantIndicator*/ false);
		BeamVfxComponent->Activate(true);
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::BeamLength, BeamRange);
		BeamVfxComponent->SetVariableFloat(GeoNiagaraParams::BeamWidth, BeamHalfWidth * 2.f);
		BeamVfxComponent->SetColorParameter(GeoNiagaraParams::Color, BeamColor.GetColor());
	}

	Super::StartPattern();
}

void UBeamPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	MoveBeamVfx(SpentTime);

	if (GeoLib::IsServer(GetWorld()) && CVarDrawBeamBorder.GetValueOnGameThread())
	{
		FVector const Location = GetBeamOrigin();
		FVector const Forward = FRotator(0.f, GetBeamYaw(SpentTime), 0.f).Vector();
		FVector const Right = FVector::CrossProduct(FVector::UpVector, Forward);
		FVector const BeamStart = Location;
		FVector const BeamEnd = Location + Forward * BeamRange;
		DrawDebugLine(GetWorld(), BeamStart + Right * BeamHalfWidth, BeamEnd + Right * BeamHalfWidth, FColor::Red,
					  false, 0.f);
		DrawDebugLine(GetWorld(), BeamStart - Right * BeamHalfWidth, BeamEnd - Right * BeamHalfWidth, FColor::Red,
					  false, 0.f);
		DrawDebugLine(GetWorld(), BeamEnd - Right * BeamHalfWidth, BeamEnd + Right * BeamHalfWidth, FColor::Red,
					  false, 0.f);
	}
}

float UBeamPattern::GetHazardDuration() const
{
	return BeamDuration;
}

bool UBeamPattern::IsInHazard(AActor const* Target, FVector2D const Location, float const SpentTime) const
{
	bool const bIncludeTargetRadius =
		GeoASLib::ShouldIncludeTargetRadius(OverlapMode, GeoASLib::GetTeamId(StoredPayload.SourceOwner));
	float const TargetRadius = bIncludeTargetRadius ? Target->GetSimpleCollisionRadius() : 0.f;
	FVector2D const ToTarget = Location - FVector2D(GetBeamOrigin());
	FVector2D const Forward(FRotator(0.f, GetBeamYaw(SpentTime), 0.f).Vector());
	return ToTarget.SizeSquared() <= FMath::Square(BeamRange + TargetRadius) && (ToTarget | Forward) >= 0.f
		&& FMath::Abs(ToTarget ^ Forward) <= BeamHalfWidth + TargetRadius;
}

void UBeamPattern::OnHazardEnd()
{
	Super::OnHazardEnd();
	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->Deactivate();
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
	if (bForceStop && IsPatternActive() && IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->DeactivateImmediate();
	}

	Super::EndPattern(bForceStop);
}
