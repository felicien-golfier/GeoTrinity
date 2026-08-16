// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSpawnPillarAbility.h"

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

TInstancedStruct<FPatternData> UGeoSpawnPillarAbility::CreatePatternData() const
{
	FSpawnPillarPatternData PillarData;

	UGeoAbilitySystemComponent* GeoAsc = GeoASLib::GetGeoAscFromActor(StoredPayload.Owner);
	if (!ensureMsgf(IsValid(GeoAsc), TEXT("GeoSpawnPillarAbility: Owner has no ASC")))
	{
		return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
	}

	UGeoAttributeSetBase const* AttributeSet =
		Cast<UGeoAttributeSetBase>(GeoAsc->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	if (!ensureMsgf(IsValid(AttributeSet), TEXT("GeoSpawnPillarAbility: OwnerASC has no UGeoAttributeSetBase")))
	{
		return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
	}

	float const HealthRatio = AttributeSet->GetHealthRatio();
	int32 const NumPillarToSpawn = HealthRatio < .2f ? 3 : HealthRatio < .5f ? 2 : 1;

	TArray<APlayableCharacter*> const AlivePlayers = GeoLib::GetAlivePlayers(this);
	if (AlivePlayers.IsEmpty())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("GeoSpawnPillarAbility: no alive player to target"));
		return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
	}

	FRandomStream Stream(StoredPayload.Seed);
	int32 const FirstPlayerIndex = Stream.RandHelper(AlivePlayers.Num());
	for (int32 Index = 0; Index < NumPillarToSpawn; ++Index)
	{
		FVector2D Location(AlivePlayers[(FirstPlayerIndex + Index) % AlivePlayers.Num()]->GetActorLocation());
		if (Index >= AlivePlayers.Num())
		{
			float const RandomAngle = Stream.FRandRange(0.f, 2.f * PI);
			float const RandomRadius = Stream.FRandRange(MinScatterRadius, MaxScatterRadius);
			Location += FVector2D(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle)) * RandomRadius;
		}
		PillarData.ZoneLocations.Add(Location);
	}

	return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
}
