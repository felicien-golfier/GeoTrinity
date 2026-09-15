// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/BeamPattern.h"

#include "AbilitySystem/Abilities/Boss/GeoSweepBeamAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

static TAutoConsoleVariable CVarDrawBeamBorder(TEXT("Geo.DrawBeamBorder"), false,
											   TEXT("When true, draws the beam pattern's hit-scan rectangle borders"));

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
	AnglesFromBeam.Reset();

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

void UBeamPattern::TickPattern(float const ServerTime, float const SpentTime)
{
	MoveBeamVfx(SpentTime);

	float OldestActorSpentTime = SpentTime;
	if (GeoLib::IsServer(GetWorld()))
	{
		FVector const Location = GetBeamOrigin();
		FVector2D const Forward(FRotator(0.f, GetBeamYaw(SpentTime), 0.f).Vector());

		if (CVarDrawBeamBorder.GetValueOnGameThread())
		{
			FVector const Right = FVector::CrossProduct(FVector::UpVector, FVector(Forward, 0.f));
			FVector const BeamStart = Location;
			FVector const BeamEnd = Location + FVector(Forward, 0.f) * BeamRange;
			DrawDebugLine(GetWorld(), BeamStart + Right * BeamHalfWidth, BeamEnd + Right * BeamHalfWidth, FColor::Red,
						  false, 0.f);
			DrawDebugLine(GetWorld(), BeamStart - Right * BeamHalfWidth, BeamEnd - Right * BeamHalfWidth, FColor::Red,
						  false, 0.f);
			DrawDebugLine(GetWorld(), BeamEnd - Right * BeamHalfWidth, BeamEnd + Right * BeamHalfWidth, FColor::Red,
						  false, 0.f);
		}

		UGeoAbilitySystemComponent* const SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.SourceOwner);
		// A missing ASC only costs the damage: falling through still lets the beam reach its end and stop ticking.
		if (ensureMsgf(SourceASC, TEXT("UBeamPattern: Owner has no ASC")))
		{
			FGenericTeamId const SourceTeam = GeoASLib::GetTeamId(StoredPayload.SourceOwner);
			bool const bIncludeTargetRadius = GeoASLib::ShouldIncludeTargetRadius(OverlapMode, SourceTeam);
			TMap<TWeakObjectPtr<AActor>, float> const PreviousAnglesFromBeam = MoveTemp(AnglesFromBeam);
			TArray<AActor*> ActorsInBeam;
			TArray<AActor*> ActorsEnteringBeam;
			for (AActor* Actor :
				 GeoASLib::GetInteractableActors(this, SourceTeam, TeamAttitudeMask::HostileOrNeutral,
												 /*bMustBeDamageable*/ true, FVector2D(Location), BeamRange, OverlapMode))
			{
				float const ActorSpentTime = SpentTime - (ServerTime - GeoLib::GetPerceivedServerTime(Actor));
				OldestActorSpentTime = FMath::Min(OldestActorSpentTime, ActorSpentTime);
				if (ActorSpentTime < 0.f || ActorSpentTime > BeamDuration)
				{
					continue;
				}

				FVector2D const ToActor = FVector2D(Actor->GetActorLocation()) - FVector2D(Location);
				float const HitRadius =
					BeamHalfWidth + (bIncludeTargetRadius ? Actor->GetSimpleCollisionRadius() : 0.f);
				float const HalfAngle =
					FMath::RadiansToDegrees(FMath::Asin(HitRadius / FMath::Max(ToActor.Size(), HitRadius)));
				float const AngleFromBeam = FMath::FindDeltaAngleDegrees(
					GetBeamYaw(ActorSpentTime), FMath::RadiansToDegrees(FMath::Atan2(ToActor.Y, ToActor.X)));
				AnglesFromBeam.Add(Actor, AngleFromBeam);

				if (FMath::Abs(AngleFromBeam) <= HalfAngle)
				{
					ActorsInBeam.Add(Actor);
				}

				float const* const PreviousAngleFromBeam = PreviousAnglesFromBeam.Find(Actor);
				if (PreviousAngleFromBeam && FMath::Abs(*PreviousAngleFromBeam) <= HalfAngle)
				{
					continue;
				}

				float const SweptFromAngle = PreviousAngleFromBeam
					? AngleFromBeam + FMath::FindDeltaAngleDegrees(AngleFromBeam, *PreviousAngleFromBeam)
					: AngleFromBeam;
				if (FMath::Max(SweptFromAngle, AngleFromBeam) >= -HalfAngle
					&& FMath::Min(SweptFromAngle, AngleFromBeam) <= HalfAngle)
				{
					ActorsEnteringBeam.Add(Actor);
				}
			}

			ApplyBeamEffects(/*bPerSecond*/ true, ActorsInBeam, SourceASC);
			if (!bPatternIsActive)
			{
				return;
			}

			ApplyBeamEffects(/*bPerSecond*/ false, ActorsEnteringBeam, SourceASC);
		}
	}

	if (OldestActorSpentTime >= BeamDuration)
	{
		EndPattern();
	}
}

void UBeamPattern::ApplyBeamEffects(bool const bPerSecond, TArray<AActor*> const& Actors,
									UGeoAbilitySystemComponent* SourceASC) const
{
	TArray<TInstancedStruct<FEffectData>> const Effects = EffectDataArray.FilterByPredicate(
		[bPerSecond](TInstancedStruct<FEffectData> const& Effect)
		{
			FEffectData const* const EffectData = Effect.GetPtr();
			return EffectData && EffectData->IsPerSecond() == bPerSecond;
		});

	if (Effects.IsEmpty())
	{
		return;
	}

	for (AActor* const HitActor : Actors)
	{
		if (UGeoAbilitySystemComponent* const TargetASC = GeoASLib::GetGeoAscFromActor(HitActor))
		{
			GeoASLib::ApplyEffectFromEffectData(Effects, SourceASC, TargetASC, StoredPayload.AbilityLevel,
												StoredPayload.Seed, StoredPayload.AbilityTag);
			GeoASLib::NotifyAbilityHit(StoredPayload, HitActor);
		}

		if (!bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
		{
			return;
		}
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
	if (IsPatternActive() && IsValid(BeamVfxComponent))
	{
		// A force-stopped beam must vanish at once; a natural end can play out its fade.
		if (bForceStop)
		{
			BeamVfxComponent->DeactivateImmediate();
		}
		else
		{
			BeamVfxComponent->Deactivate();
		}
	}

	Super::EndPattern(bForceStop);
}
