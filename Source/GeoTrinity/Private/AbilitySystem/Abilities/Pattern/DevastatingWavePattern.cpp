// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/DevastatingWavePattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "DrawDebugHelpers.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void UDevastatingWavePattern::InitPattern(FAbilityPayload const& Payload)
{
	Super::InitPattern(Payload);
	if (!IsValid(StoredPayload.Owner))
	{
		ensureMsgf(false, TEXT("UDevastatingWavePattern: StoredPayload.Owner is null"));
		return;
	}

	HitActors.Empty();
	PillarsWaveData.Empty();

	StoredPayload.Instigator->SetActorLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));
}

FGameplayCueParameters UDevastatingWavePattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = MaxRadius;
	float const LifeTime = MaxRadius / ExpansionSpeed;
	CueParams.Normal = FVector(LifeTime, StartTime, 1.f - (LifeTime - StartTime) / LifeTime);
	return CueParams;
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

void UDevastatingWavePattern::TickPattern(float ServerTime, float SpentTime)
{
	float const CurrentRadius = ExpansionSpeed * SpentTime;

	if (UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
		if (ensureMsgf(SourceASC, TEXT("UDevastatingWavePattern: SourceASC is null — Owner has no ASC")))
		{
			for (AActor* HitActor : GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
																	TeamAttitudeMask::HostileOrNeutral, true,
																	StoredPayload.Origin, CurrentRadius,
																	[this](AActor const* Actor)
																	{
																		return ShouldHitActor(Actor);
																	}))
			{
				if (HitActors.Contains(HitActor))
				{
					continue;
				}
				HitActors.Add(HitActor);

				if (AGeoPillar* Pillar = Cast<AGeoPillar>(HitActor))
				{
					PillarsWaveData.Add(
						{FVector2D(Pillar->GetActorLocation()), Pillar->GetSimpleCollisionRadius(), Pillar});
				}
				else if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(HitActor))
				{
					UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
																		StoredPayload.AbilityLevel, StoredPayload.Seed);
				}
			}
		}
	}

#if WITH_EDITOR
	DrawDebugSafeZones(CurrentRadius);
#endif

	if (CurrentRadius >= MaxRadius)
	{
		EndPattern();
	}
}

void UDevastatingWavePattern::EndPattern()
{
	if (!UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		Super::EndPattern();
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (!SourceASC)
	{
		ensureMsgf(false, TEXT("UDevastatingWavePattern: SourceASC is null on wave end — Owner has no ASC"));
		Super::EndPattern();
		return;
	}

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
															StoredPayload.AbilityLevel, StoredPayload.Seed);
	}

	Super::EndPattern();
}
