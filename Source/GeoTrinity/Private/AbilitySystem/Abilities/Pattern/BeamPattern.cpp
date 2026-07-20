// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/BeamPattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/GeoHexArena.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

namespace
{
	FName const BeamLengthParamName{"User.Beam_Length"};
	FName const BeamWidthParamName{"User.Beam_Width"};
} // namespace

void UBeamPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (BeamVfxSystem && !GeoLib::IsDedicatedServer(GetWorld()))
	{
		BeamVfxComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, BeamVfxSystem, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ false, /*bAutoActivate*/ false);
		ensureMsgf(BeamVfxComponent, TEXT("UBeamPattern: failed to spawn the beam VFX system"));
	}
}

float UBeamPattern::GetBeamYaw(float const SpentTime) const
{
	float const SweptFraction = FMath::Clamp(SpentTime / BeamDuration, 0.f, 1.f);
	return StoredPayload.Yaw - SweepAngle * 0.5f + SweepAngle * SweptFraction;
}

void UBeamPattern::StartPattern()
{
	HitActors.Empty();

	if (bDestroyLastTileHit && GeoLib::IsServer(GetWorld()))
	{
		AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner);
		FVector2D const Forward(FRotator(0.f, GetBeamYaw(0.f), 0.f).Vector());
		FIntPoint LastTile;
		if (ensureMsgf(Arena, TEXT("UBeamPattern: %s is not a hex arena boss"), *GetNameSafe(StoredPayload.Owner))
			&& Arena->GetLastAliveTileAlongRay(StoredPayload.Origin, Forward, BeamRange, LastTile))
		{
			Arena->DestroyTiles({LastTile});
		}
	}

	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->SetVariableFloat(BeamLengthParamName, BeamRange);
		BeamVfxComponent->SetVariableFloat(BeamWidthParamName, BeamHalfWidth);
		BeamVfxComponent->Activate(true);
	}

	Super::StartPattern();
}

void UBeamPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	FRotator const BeamRotation(0.f, GetBeamYaw(SpentTime), 0.f);
	FVector2D const Forward(BeamRotation.Vector());

	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->SetWorldLocationAndRotation(FVector(StoredPayload.Origin, ArbitraryCharacterZ), BeamRotation);
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		UGeoAbilitySystemComponent* const SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
		// A missing ASC only costs the damage: falling through still lets the beam reach its end and stop ticking.
		if (ensureMsgf(SourceASC, TEXT("UBeamPattern: Owner has no ASC")))
		{
			for (AActor* HitActor : GeoASLib::GetInteractableActorsInLine(
					 this, GeoASLib::GetTeamId(StoredPayload.Owner), TeamAttitudeMask::HostileOrNeutral,
					 /*bMustBeDamageable*/ true, StoredPayload.Origin, Forward, BeamRange, BeamHalfWidth))
			{
				if (HitActors.Contains(HitActor))
				{
					continue;
				}
				HitActors.Add(HitActor);

				if (UGeoAbilitySystemComponent* const TargetASC = GeoASLib::GetGeoAscFromActor(HitActor))
				{
					GeoASLib::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
														StoredPayload.AbilityLevel, StoredPayload.Seed,
														StoredPayload.AbilityTag);
				}

				if (!bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
				{
					return;
				}
			}
		}
	}

	if (SpentTime >= BeamDuration)
	{
		EndPattern();
	}
}

FGameplayCueParameters UBeamPattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = BeamRange;
	return CueParams;
}

void UBeamPattern::EndPattern(bool const bForceStop)
{
	// UPatternAbility::EndAbility force-ends the pattern a second time right after a natural end; the IsPatternActive
	// guard keeps that redundant call from cutting the graceful fade short with DeactivateImmediate.
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

	HitActors.Empty();
	Super::EndPattern(bForceStop);
}
