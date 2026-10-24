// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/DevastatingWavePattern.h"

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotSource.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "GeoTrinity/GeoTrinity.h"
#include "NiagaraComponent.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "Tool/Visual/GeoNiagaraParams.h"

static TAutoConsoleVariable
	CVarDrawDevastatingWave(TEXT("Geo.DrawDevastatingWave"), false,
							TEXT("When true, draws the devastating wave's front / inner radius and pillar safe zones"));

UDevastatingWavePattern::UDevastatingWavePattern()
{
	bHasHazard = true;
}

void UDevastatingWavePattern::InitPattern(FAbilityPayload const& Payload,
										  TInstancedStruct<FPatternData> const& PatternData)
{
	Super::InitPattern(Payload, PatternData);
	if (!ensureMsgf(IsValid(StoredPayload.SourceOwner), TEXT("%hs: StoredPayload.SourceOwner is null"), __FUNCTION__))
	{
		return;
	}

	ClearData();

	StoredPayload.SourceAvatar->SetActorLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));

	// Skipped on the "too late" path (Super::InitPattern ran StartPattern synchronously, no wind-up scheduled).
	if (IsValid(AOEVfxComponent) && StartSectionTimerHandle.IsValid())
	{
		AddPillarSpots(0.f);
		ActivateAoeVfxTelegraph();
	}
}

void UDevastatingWavePattern::AddPillarSpots(float const MaxDistance)
{
	bool bAdded = false;
	for (FSafeSpot const& Spot : GatherSafeSpots(this, StoredPayload, {ESafeSpotSource::Pillars},
												 ESafeSpotShadow::Behind, MaxDistance, 0.f, {}))
	{
		if (!PillarSpots.ContainsByPredicate(
				[&Spot](FSafeSpot const& Known)
				{
					return Known.Source == Spot.Source;
				}))
		{
			PillarSpots.Add(Spot);
			bAdded = true;
		}
	}

	if (bAdded)
	{
		FSafeSpotMask::Set(this, MaskMaterialParameterCollection, PillarSpots, FVector2D::ZeroVector);
	}
}

void UDevastatingWavePattern::OnCreate(FGameplayTag AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (!GeoLib::IsDedicatedServer(GetWorld())
		&& ensureMsgf(AOEVfxSystem && MaskMaterialParameterCollection,
					  TEXT("UDevastatingWavePattern: AOEVfxSystem or MaskMaterialParameterCollection is not set")))
	{
		AOEVfxComponent = SpawnPersistentVfx(AOEVfxSystem, Owner);
	}
}

void UDevastatingWavePattern::ClearData()
{
	PillarSpots.Empty();
	// The MPC is global state — clear slots left over from a previous wave before the AOE starts rendering.
	FSafeSpotMask::Clear(this, MaskMaterialParameterCollection);
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

FGameplayCueParameters UDevastatingWavePattern::FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Cue, Payload);
	CueParams.RawMagnitude = MaxRadius;
	float const LifeTime = MaxRadius / ExpansionSpeed;
	CueParams.Normal = FVector(LifeTime, TravelTime, 1.f - (LifeTime - TravelTime) / LifeTime);
	return CueParams;
}

void UDevastatingWavePattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	float const CurrentRadius = FMath::Min(ExpansionSpeed * SpentTime, MaxRadius);
	if (CurrentRadius <= 0.f)
	{
		return;
	}

	AddPillarSpots(CurrentRadius);

	if (CVarDrawDevastatingWave.GetValueOnGameThread())
	{
		DrawDebugWave(CurrentRadius);
	}
}

float UDevastatingWavePattern::GetHazardDuration() const
{
	return MaxRadius / ExpansionSpeed;
}

bool UDevastatingWavePattern::IsInHazard(AActor const* /*Target*/, FVector2D const Location,
										 float const SpentTime) const
{
	float const WaveRadius = FMath::Min(ExpansionSpeed * SpentTime, MaxRadius);
	float const FrontOffset = WaveRadius - FVector2D::Distance(StoredPayload.Origin, Location);
	return FrontOffset >= 0.f && FrontOffset <= AnnulusWidth
		&& !FSafeSpot::AnyShields(PillarSpots, Location, StoredPayload.Origin, FVector2D::ZeroVector);
}

void UDevastatingWavePattern::OnHazardEnd()
{
	Super::OnHazardEnd();
	if (IsValid(AOEVfxComponent))
	{
		AOEVfxComponent->Deactivate();
	}
}

void UDevastatingWavePattern::DrawDebugWave(float CurrentRadius) const
{
	FVector const Center(StoredPayload.Origin, ArbitraryCharacterZ);
	DrawDebugCircle(GetWorld(), Center, CurrentRadius, 64, FColor::Red, false, 0.f, 0, 3.f, FVector::XAxisVector,
					FVector::YAxisVector, false);
	DrawDebugCircle(GetWorld(), Center, FMath::Max(0.f, CurrentRadius - AnnulusWidth), 64, FColor::Green, false, 0.f, 0,
					3.f, FVector::XAxisVector, FVector::YAxisVector, false);

	int i = 0;
	for (FSafeSpot const& PillarSpot : PillarSpots)
	{
		FVector2D const PillarLocation(PillarSpot.Location);
		float const PillarRadius = PillarSpot.Radius;
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
	if (bForceStop && IsValid(AOEVfxComponent))
	{
		AOEVfxComponent->DeactivateImmediate();
	}

	if (!UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		Super::EndPattern(bForceStop);
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.SourceOwner);
	if (!ensureMsgf(SourceASC, TEXT("%hs: Owner has no ASC on wave end"), __FUNCTION__))
	{
		Super::EndPattern(bForceStop);
		return;
	}

	if (!bForceStop)
	{
		for (FSafeSpot const& PillarSpot : PillarSpots)
		{
			if (!PillarSpot.Source.IsValid())
			{
				continue;
			}
			UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(PillarSpot.Source.Get());
			if (!ensureMsgf(TargetASC, TEXT("%hs: alive pillar has no ASC"), __FUNCTION__))
			{
				continue;
			}
			FLethalEffectData LethalEffectData;
			UGeoAbilitySystemLibrary::ApplySingleEffectData(LethalEffectData, SourceASC, TargetASC,
															StoredPayload.AbilityLevel, StoredPayload.Seed,
															StoredPayload.AbilityTag);
			UGeoAbilitySystemLibrary::NotifyAbilityHit(StoredPayload, PillarSpot.Source.Get());
		}
	}

	ClearData();

	Super::EndPattern(bForceStop);
}
