// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/DevastatingWavePattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "DrawDebugHelpers.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

namespace
{
	constexpr int32 MaxMaskedPillarSlots = 8;
	// Matches the "unused slot" default of MPC_MaskedArea's PillarPosWS_XX parameters.
	constexpr FLinearColor UnusedPillarSlotValue(-10000.f, -10000.f, -10000.f, 0.f);

	FName GetPillarSlotParameterName(int32 const SlotIndex)
	{
		return FName(FString::Printf(TEXT("PillarPosWS_%02d"), SlotIndex));
	}
} // namespace

void UDevastatingWavePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);
	if (!IsValid(StoredPayload.Owner))
	{
		ensureMsgf(false, TEXT("UDevastatingWavePattern: StoredPayload.Owner is null"));
		return;
	}

	HitActors.Empty();
	PillarsWaveData.Empty();

	StoredPayload.Instigator->SetActorLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));
}

void UDevastatingWavePattern::OnCreate(FGameplayTag AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (GeoLib::IsDedicatedServer(GetWorld())
		|| !ensureMsgf(AOEVfxSystem && MaskMaterialParameterCollection,
					   TEXT("UDevastatingWavePattern: AOEVfxSystem or MaskMaterialParameterCollection is not set")))
	{
		return;
	}

	AOEVfxComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AOEVfxSystem, FVector::ZeroVector,
																	 FRotator::ZeroRotator, FVector::OneVector,
																	 /*bAutoDestroy*/ false, /*bAutoActivate*/ false);
	ensureMsgf(AOEVfxComponent, TEXT("UDevastatingWavePattern: failed to spawn the AOE VFX system"));
}

void UDevastatingWavePattern::StartPattern()
{
	Super::StartPattern();

	if (!IsValid(AOEVfxComponent))
	{
		return;
	}

	// The MPC is global state — clear slots left over from a previous wave before the AOE starts rendering.
	for (int32 SlotIndex = 0; SlotIndex < MaxMaskedPillarSlots; ++SlotIndex)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, MaskMaterialParameterCollection,
														GetPillarSlotParameterName(SlotIndex), UnusedPillarSlotValue);
	}

	AOEVfxComponent->SetWorldLocation(FVector(StoredPayload.Origin, 0.f));
	AOEVfxComponent->SetVariableFloat(TEXT("User.AOE_Radius"), MaxRadius);
	AOEVfxComponent->SetVariableFloat(TEXT("User.AOE_GrowDuration"), MaxRadius / ExpansionSpeed);
	AOEVfxComponent->SetVariableFloat(TEXT("User.FadeOut_Duration"), FadeOutDuration);
	AOEVfxComponent->SetVariableLinearColor(TEXT("User.AOE_Color"), AOEColor);
	AOEVfxComponent->Activate(true);
}

void UDevastatingWavePattern::AddPillarToVfxMask()
{
	if (GeoLib::IsDedicatedServer(GetWorld()) || !MaskMaterialParameterCollection)
	{
		return;
	}

	int32 const SlotIndex = PillarsWaveData.Num() - 1;
	if (SlotIndex >= MaxMaskedPillarSlots)
	{
		UE_LOG(LogPattern, Warning,
			   TEXT("UDevastatingWavePattern: more pillars hit than MPC mask slots (%d), skipping"),
			   MaxMaskedPillarSlots);
		return;
	}

	FPillarWaveData const& PillarData = PillarsWaveData[SlotIndex];
	UKismetMaterialLibrary::SetVectorParameterValue(this, MaskMaterialParameterCollection,
													GetPillarSlotParameterName(SlotIndex),
													FLinearColor(PillarData.Location.X, PillarData.Location.Y, 0.f));
	UKismetMaterialLibrary::SetScalarParameterValue(this, MaskMaterialParameterCollection, TEXT("Pillar_Radius"),
													PillarData.Radius);
}

FGameplayCueParameters UDevastatingWavePattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = MaxRadius;
	float const LifeTime = MaxRadius / ExpansionSpeed;
	CueParams.Normal = FVector(LifeTime, StartTime, 1.f - (LifeTime - StartTime) / LifeTime);
	return CueParams;
}

void UDevastatingWavePattern::TickPattern(float ServerTime, float SpentTime)
{
	float const CurrentRadius = ExpansionSpeed * SpentTime;
	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (ensureMsgf(SourceASC, TEXT("UDevastatingWavePattern: SourceASC is null — Owner has no ASC")))
	{
		for (AActor* HitActor : GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
																TeamAttitudeMask::HostileOrNeutral, true,
																StoredPayload.Origin, CurrentRadius))
		{
			if (HitActors.Contains(HitActor))
			{
				continue;
			}
			HitActors.Add(HitActor);

			AGeoPillar* Pillar = Cast<AGeoPillar>(HitActor);
			if (IsValid(Pillar))
			{

				PillarsWaveData.Add(
					{FVector2D(Pillar->GetActorLocation()), Pillar->GetSimpleCollisionRadius(), Pillar});
				AddPillarToVfxMask();
			}

			if (GeoLib::IsServer(this) && ShouldHitActor(HitActor))
			{
				if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(HitActor))
				{
					UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
																		StoredPayload.AbilityLevel, StoredPayload.Seed,
																		StoredPayload.AbilityTag);
				}
			}
		}
	}

	if (CurrentRadius >= MaxRadius)
	{
		EndPattern();
	}
}

bool UDevastatingWavePattern::ShouldHitActor(AActor const* Actor) const
{
	for (FPillarWaveData const& PillarData : PillarsWaveData)
	{
		FVector2D const ActorLocation(Actor->GetActorLocation());
		FVector2D const CenterToActor(ActorLocation - StoredPayload.Origin);
		FVector2D const CenterToPillar(PillarData.Location - StoredPayload.Origin);
		float const Dot = CenterToActor | CenterToPillar;
		float const CenterToPillarSizeSquared = CenterToPillar.SizeSquared();

		if (Dot < CenterToPillarSizeSquared) // Before the Pillar
		{
			continue;
		}

		FVector2D const ActorProjectedOnCenterToPillar = CenterToPillar * Dot / CenterToPillarSizeSquared;
		float const DistanceSquaredToPillarVector = (ActorProjectedOnCenterToPillar - CenterToActor).SizeSquared();

		if (DistanceSquaredToPillarVector < PillarData.Radius * PillarData.Radius)
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR
void UDevastatingWavePattern::DrawDebugSafeZones(float CurrentRadius) const
{
	int i = 0;
	for (FPillarWaveData const& PillarData : PillarsWaveData)
	{
		FVector2D const PillarLocation(PillarData.Location);
		float const PillarRadius = PillarData.Radius;
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
#endif
void UDevastatingWavePattern::EndPattern(bool bForceStop)
{
	if (IsValid(AOEVfxComponent))
	{
		// Deactivate() lets live particles finish their full lifetime, which spans the whole grow+fade —
		// fine when the wave ends naturally, but a force-stopped wave must vanish right away.
		if (bForceStop)
		{
			AOEVfxComponent->DeactivateImmediate();
		}
		else
		{
			AOEVfxComponent->Deactivate();
		}
	}

	if (!UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		Super::EndPattern(bForceStop);
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (!SourceASC)
	{
		ensureMsgf(false, TEXT("UDevastatingWavePattern: SourceASC is null on wave end — Owner has no ASC"));
		Super::EndPattern(bForceStop);
		return;
	}

	if (!bForceStop)
	{
		for (FPillarWaveData const& PillarData : PillarsWaveData)
		{
			if (!PillarData.Pillar.IsValid())
			{
				continue;
			}
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(PillarData.Pillar.Get());
			if (!TargetASC)
			{
				ensureMsgf(false, TEXT("UDevastatingWavePattern: alive pillar has no ASC"));
				continue;
			}
			UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
																StoredPayload.AbilityLevel, StoredPayload.Seed,
																StoredPayload.AbilityTag);
		}
	}

	Super::EndPattern(bForceStop);
}
