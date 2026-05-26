// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/DevastatingWavePattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "DrawDebugHelpers.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void UDevastatingWavePattern::StartPattern()
{
	if (!IsValid(StoredPayload.Owner))
	{
		ensureMsgf(false, TEXT("UDevastatingWavePattern: StoredPayload.Owner is null"));
		return;
	}

	HitActors.Empty();
	PillarsLocationAndRadius.Empty();
	Super::StartPattern();

	StoredPayload.Instigator->SetActorLocation(FVector(StoredPayload.Origin, ArbitraryCharacterZ));
}

bool UDevastatingWavePattern::ShouldHitActor(AActor const* Actor) const
{
	for (auto const PillarLocationAndRadius : PillarsLocationAndRadius)
	{
		FVector2D const PillarLocation(PillarLocationAndRadius.Key);
		FVector2D const ActorLocation(Actor->GetActorLocation());
		FVector2D const CenterToActor(ActorLocation - StoredPayload.Origin);
		FVector2D const CenterToPillar(PillarLocation - StoredPayload.Origin);
		float const Dot = CenterToActor | CenterToPillar;
		float const CenterToPillarSizeSquared = CenterToPillar.SizeSquared();

		if (Dot < CenterToPillarSizeSquared) // Before the Pillar
		{
			continue;
		}

		FVector2D const ActorProjectedOnCenterToPillar = CenterToPillar * Dot / CenterToPillarSizeSquared;
		float const DistanceSquaredToPillarVector = (ActorProjectedOnCenterToPillar - CenterToActor).SizeSquared();

		float const PillarRadius = PillarLocationAndRadius.Value;

		if (DistanceSquaredToPillarVector < PillarRadius * PillarRadius)
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR
void UDevastatingWavePattern::DrawDebugSafeZones() const
{
	for (auto const& PillarLocationAndRadius : PillarsLocationAndRadius)
	{
		FVector2D const PillarLocation(PillarLocationAndRadius.Key);
		float const PillarRadius = PillarLocationAndRadius.Value;
		FVector2D const CenterToPillar(PillarLocation - StoredPayload.Origin);
		float const PillarDistance = CenterToPillar.Size();
		FVector2D const PillarDir(CenterToPillar / PillarDistance);
		FVector2D const PillarPerp(-PillarDir.Y, PillarDir.X);

		FVector2D const LeftTangent(PillarLocation + PillarPerp * PillarRadius);
		FVector2D const RightTangent(PillarLocation - PillarPerp * PillarRadius);

		FVector2D const LeftEnd(StoredPayload.Origin
								+ (LeftTangent - StoredPayload.Origin).GetSafeNormal() * MaxRadius);
		FVector2D const RightEnd(StoredPayload.Origin
								 + (RightTangent - StoredPayload.Origin).GetSafeNormal() * MaxRadius);

		FColor const Color = GeoLib::GetRandomColorFromPalette();
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
					PillarsLocationAndRadius.Add(
						{FVector2D(Pillar->GetActorLocation()), Pillar->GetSimpleCollisionRadius()});
					Pillar->Recall(1.f);
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
	DrawDebugSafeZones();
#endif

	if (CurrentRadius >= MaxRadius)
	{
		EndPattern();
	}
}
