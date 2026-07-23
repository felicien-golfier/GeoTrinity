// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/BeamPattern.h"

#include "AbilitySystem/Abilities/Boss/GeoSweepBeamAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/GeoHexArena.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"


void UBeamPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (UGeoSweepBeamAbility const* SweepBeamAbility = GeoASLib::GetAbilityCDO<UGeoSweepBeamAbility>(AbilityTag))
	{
		SweepAngle = SweepBeamAbility->GetSweepAngle();
	}

	if (BeamVfxSystem && !GeoLib::IsDedicatedServer(GetWorld()))
	{
		BeamVfxComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, BeamVfxSystem, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ false, /*bAutoActivate*/ false);
		ensureMsgf(BeamVfxComponent, TEXT("UBeamPattern: failed to spawn the beam VFX system"));
		BeamVfxComponent->SetColorParameter(BeamColorParamName, BeamColor);
	}
}
void UBeamPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);

	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->Activate(true);
		BeamVfxComponent->SetVariableFloat(BeamLengthParamName, BeamRange);
		BeamVfxComponent->SetVariableFloat(BeamWidthParamName, BeamHalfWidth);
		BeamVfxComponent->SetColorParameter(BeamColorParamName, BeamInitColor);
		FRotator const BeamRotation(0.f, GetBeamYaw(0.f), 0.f);
		FVector const Location = FollowBossLocation ? StoredPayload.Instigator->GetActorLocation()
													: FVector(StoredPayload.Origin, ArbitraryCharacterZ);
		if (IsValid(BeamVfxComponent))
		{
			BeamVfxComponent->SetWorldLocationAndRotation(Location, BeamRotation);
		}
	}
}

float UBeamPattern::GetBeamYaw(float const SpentTime) const
{
	if (FollowBossOrientation && IsValid(StoredPayload.Instigator))
	{
		return StoredPayload.Instigator->GetActorRotation().Yaw;
	}

	float const SweptFraction = FMath::Clamp(SpentTime / BeamDuration, 0.f, 1.f);
	float const SweepSign = StoredPayload.Seed % 2 == 0 ? 1.f : -1.f;
	return StoredPayload.Yaw - SweepSign * (2.f * SweepAngle * SweptFraction);
}

void UBeamPattern::StartPattern()
{
	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->Activate(true);
		BeamVfxComponent->SetVariableFloat(BeamLengthParamName, BeamRange);
		BeamVfxComponent->SetVariableFloat(BeamWidthParamName, BeamHalfWidth);
		BeamVfxComponent->SetColorParameter(BeamColorParamName, BeamColor);
	}

	Super::StartPattern();
}

void UBeamPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	FRotator const BeamRotation(0.f, GetBeamYaw(SpentTime), 0.f);
	FVector2D const Forward(BeamRotation.Vector());
	FVector Location = FollowBossLocation ? StoredPayload.Instigator->GetActorLocation()
										  : FVector(StoredPayload.Origin, ArbitraryCharacterZ);
	if (IsValid(BeamVfxComponent))
	{
		BeamVfxComponent->SetWorldLocationAndRotation(Location, BeamRotation);
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		if (bDestroyLastTileHit)
		{
			AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner);
			FIntPoint LastTile;
			if (ensureMsgf(Arena, TEXT("UBeamPattern: %s is not a hex arena boss"), *GetNameSafe(StoredPayload.Owner))
				&& Arena->GetLastAliveTileAlongRay(FVector2D(Location), Forward, LastTile))
			{
				Arena->HighlightTile(StoredPayload.Instigator, LastTile);
			}
		}

		UGeoAbilitySystemComponent* const SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
		// A missing ASC only costs the damage: falling through still lets the beam reach its end and stop ticking.
		if (ensureMsgf(SourceASC, TEXT("UBeamPattern: Owner has no ASC")))
		{
			for (AActor* HitActor : GeoASLib::GetInteractableActorsInLine(
					 this, GeoASLib::GetTeamId(StoredPayload.Owner), TeamAttitudeMask::HostileOrNeutral,
					 /*bMustBeDamageable*/ true, FVector2D(Location), Forward, BeamRange, BeamHalfWidth))
			{
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
	if (IsPatternActive())
	{
		if (!bForceStop && bDestroyLastTileHit && GeoLib::IsServer(GetWorld()))
		{
			AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner);
			FVector2D const Forward(FRotator(0.f, GetBeamYaw(0.f), 0.f).Vector());
			FIntPoint LastTile;
			FVector Location = FollowBossLocation ? StoredPayload.Instigator->GetActorLocation()
												  : FVector(StoredPayload.Origin, ArbitraryCharacterZ);
			if (ensureMsgf(Arena, TEXT("UBeamPattern: %s is not a hex arena boss"), *GetNameSafe(StoredPayload.Owner))
				&& Arena->GetLastAliveTileAlongRay(FVector2D(Location), Forward, LastTile))
			{
				Arena->DestroyTiles({LastTile});
			}
		}

		if (IsValid(BeamVfxComponent))
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
	}

	Super::EndPattern(bForceStop);
}
