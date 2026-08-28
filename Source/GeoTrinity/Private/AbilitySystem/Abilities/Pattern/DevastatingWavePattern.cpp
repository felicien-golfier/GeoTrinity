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
#include "Tool/GeoNiagaraParams.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

static TAutoConsoleVariable
	CVarDrawDevastatingWave(TEXT("Geo.DrawDevastatingWave"), false,
							TEXT("When true, draws the devastating wave's front / inner radius and pillar safe zones"));

namespace
{
	constexpr int32 MaxMaskedPillarSlots = 8;
	// Matches the "unused slot" default of MPC_MaskedArea's PillarPosWS_XX parameters.
	constexpr FLinearColor UnusedPillarSlotValue(-10000.f, -10000.f, -10000.f, 0.f);

	FName const PillarRadiusParam(TEXT("Pillar_Radius"));

	FName GetPillarSlotParameterName(int32 const SlotIndex)
	{
		return FName(FString::Printf(TEXT("PillarPosWS_%02d"), SlotIndex));
	}
} // namespace

void UDevastatingWavePattern::InitPattern(FAbilityPayload const& Payload,
										  TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);
	if (!ensureMsgf(IsValid(StoredPayload.Owner), TEXT("%hs: StoredPayload.Owner is null"), __FUNCTION__))
	{
		return;
	}

	ClearData();

	StoredPayload.Instigator->SetActorLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));

	// Skipped on the "too late" path (Super::InitPattern ran StartPattern synchronously, no wind-up scheduled).
	if (IsValid(AOEVfxComponent) && StartSectionTimerHandle.IsValid())
	{
		AddAllPillarsToVfxMask();
		ActivateAoeVfxTelegraph();
	}
}

void UDevastatingWavePattern::AddAllPillarsToVfxMask()
{
	for (AGeoPillar* Pillar :
		 GeoASLib::GetInteractableActors<AGeoPillar>(this, GeoASLib::GetTeamId(StoredPayload.Owner),
													 TeamAttitudeMask::HostileOrNeutral, true, StoredPayload.Origin, 0))
	{
		PillarsWaveData.Add({FVector2D(Pillar->GetActorLocation()), Pillar->GetSimpleCollisionRadius(), Pillar});
		AddPillarToVfxMask();
	}
}

void UDevastatingWavePattern::OnCreate(FGameplayTag AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (!GeoLib::IsDedicatedServer(GetWorld())
		&& ensureMsgf(AOEVfxSystem && MaskMaterialParameterCollection,
					  TEXT("UDevastatingWavePattern: AOEVfxSystem or MaskMaterialParameterCollection is not set")))
	{
		AOEVfxComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, AOEVfxSystem, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ false, /*bAutoActivate*/ false);
		ensureMsgf(AOEVfxComponent, TEXT("UDevastatingWavePattern: failed to spawn the AOE VFX system"));
	}
}

void UDevastatingWavePattern::ClearData()
{
	PillarsWaveData.Empty();
	// The MPC is global state — clear slots left over from a previous wave before the AOE starts rendering.
	for (int32 SlotIndex = 0; SlotIndex < MaxMaskedPillarSlots; ++SlotIndex)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, MaskMaterialParameterCollection,
														GetPillarSlotParameterName(SlotIndex), UnusedPillarSlotValue);
	}
}
void UDevastatingWavePattern::StartPattern()
{
	ClearData();

	// Leave the telegraph behind and start the real expanding wave. Re-activating keeps the same component alive,
	// so the wave origin and grow params are re-pushed here.
	GetWorld()->GetTimerManager().ClearTimer(TelegraphBlinkTimerHandle);
	if (IsValid(AOEVfxComponent))
	{
		ActivateAOEVfx();
	}

	Super::StartPattern();
}

void UDevastatingWavePattern::ActivateAoeVfxTelegraph() const
{
	AOEVfxComponent->ReinitializeSystem();
	AOEVfxComponent->SetWorldLocation(FVector(StoredPayload.Origin, 0.f));
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::AOERadius, MaxRadius);
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::AOEGrowDuration, StartDelay - TravelTime);
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::FadeOutDuration, TelegraphFadeOutDuration);
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::AnnulusRadius, MaxRadius);
	AOEVfxComponent->SetVariableLinearColor(GeoNiagaraParams::AOEColor, AOEColor.GetColor());
	AOEVfxComponent->Activate(true);
}

void UDevastatingWavePattern::ActivateAOEVfx() const
{
	AOEVfxComponent->ReinitializeSystem();
	AOEVfxComponent->SetWorldLocation(FVector(StoredPayload.Origin, 0.f));
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::AOERadius, MaxRadius);
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::AOEGrowDuration, MaxRadius / ExpansionSpeed);
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::FadeOutDuration, FadeOutDuration);
	AOEVfxComponent->SetVariableFloat(GeoNiagaraParams::AnnulusRadius, AnnulusWidth);
	AOEVfxComponent->SetVariableLinearColor(GeoNiagaraParams::AOEColor, AOEColor.GetColor());
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
	UKismetMaterialLibrary::SetScalarParameterValue(this, MaskMaterialParameterCollection, PillarRadiusParam,
													PillarData.Radius);
}

FGameplayCueParameters UDevastatingWavePattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = MaxRadius;
	float const LifeTime = MaxRadius / ExpansionSpeed;
	CueParams.Normal = FVector(LifeTime, TravelTime, 1.f - (LifeTime - TravelTime) / LifeTime);
	return CueParams;
}

void UDevastatingWavePattern::TickPattern(float ServerTime, float SpentTime)
{
	float const CurrentRadius = ExpansionSpeed * SpentTime;

	if (CurrentRadius <= 0.f)
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (ensureMsgf(SourceASC, TEXT("UDevastatingWavePattern: SourceASC is null — Owner has no ASC")))
	{
		float const InnerRadius = FMath::Max(0.f, CurrentRadius - AnnulusWidth);
		TArray<AActor*> ActorsInWaveFront;
		for (AActor* HitActor : GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
																TeamAttitudeMask::HostileOrNeutral, true,
																StoredPayload.Origin, CurrentRadius))
		{
			AGeoPillar* Pillar = Cast<AGeoPillar>(HitActor);
			if (IsValid(Pillar)
				&& !PillarsWaveData.ContainsByPredicate(
					[Pillar](FPillarWaveData const& Data)
					{
						return Data.Pillar.Get() == Pillar;
					}))
			{
				PillarsWaveData.Add(
					{FVector2D(Pillar->GetActorLocation()), Pillar->GetSimpleCollisionRadius(), Pillar});
				AddPillarToVfxMask();
			}

			if (FVector2D::DistSquared(StoredPayload.Origin, FVector2D(HitActor->GetActorLocation()))
				< InnerRadius * InnerRadius)
			{
				continue;
			}

			if (GeoLib::IsServer(this) && ShouldHitActor(HitActor))
			{
				ActorsInWaveFront.Add(HitActor);
			}
		}

		KeepActorsEnteringOverlap(ActorsInWaveFront);
		for (AActor* HitActor : ActorsInWaveFront)
		{
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(HitActor);
			if (IsValid(TargetASC))
			{
				UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
																	StoredPayload.AbilityLevel, StoredPayload.Seed,
																	StoredPayload.AbilityTag);
				UGeoAbilitySystemLibrary::NotifyAbilityHit(StoredPayload, HitActor);
			}
		}
	}

	if (CVarDrawDevastatingWave.GetValueOnGameThread())
	{
		DrawDebugWave(CurrentRadius);
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

void UDevastatingWavePattern::DrawDebugWave(float CurrentRadius) const
{
	FVector const Center(StoredPayload.Origin, ArbitraryCharacterZ);
	DrawDebugCircle(GetWorld(), Center, CurrentRadius, 64, FColor::Red, false, 0.f, 0, 3.f, FVector::XAxisVector,
					FVector::YAxisVector, false);
	DrawDebugCircle(GetWorld(), Center, FMath::Max(0.f, CurrentRadius - AnnulusWidth), 64, FColor::Green, false, 0.f, 0,
					3.f, FVector::XAxisVector, FVector::YAxisVector, false);

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

void UDevastatingWavePattern::EndPattern(bool bForceStop)
{
	GetWorld()->GetTimerManager().ClearTimer(TelegraphBlinkTimerHandle);
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
	if (!ensureMsgf(SourceASC, TEXT("%hs: Owner has no ASC on wave end"), __FUNCTION__))
	{
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
			if (!ensureMsgf(TargetASC, TEXT("%hs: alive pillar has no ASC"), __FUNCTION__))
			{
				continue;
			}
			UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
																StoredPayload.AbilityLevel, StoredPayload.Seed,
																StoredPayload.AbilityTag);
			UGeoAbilitySystemLibrary::NotifyAbilityHit(StoredPayload, PillarData.Pillar.Get());
		}
	}

	ClearData();

	Super::EndPattern(bForceStop);
}
