// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoSpawnPillarAbility.h"

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/PlayableCharacter.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoSpawnPillarAbility::FillPillarTargets()
{
	PillarTargets.Reset();

	UGeoAbilitySystemComponent const* GeoAsc = GetGeoAbilitySystemComponentFromActorInfo();
	UGeoAttributeSetBase const* AttributeSet =
		Cast<UGeoAttributeSetBase>(GeoAsc->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	if (!ensureMsgf(IsValid(AttributeSet), TEXT("GeoSpawnPillarAbility: OwnerASC has no UGeoAttributeSetBase")))
	{
		return;
	}

	float const HealthRatio = AttributeSet->GetHealthRatio();
	int32 const NumPillarToSpawn = HealthRatio < .2f ? 3 : HealthRatio < .5f ? 2 : 1;

	TArray<APlayableCharacter*> const AlivePlayers = GeoLib::GetAlivePlayers(this);
	if (AlivePlayers.IsEmpty())
	{
		UE_LOG(LogGeoASC, Warning, TEXT("GeoSpawnPillarAbility: no alive player to target"));
		return;
	}

	FRandomStream const Stream(LaunchSeed);
	int32 const FirstPlayerIndex = Stream.RandHelper(AlivePlayers.Num());

	for (int32 Index = 0; Index < NumPillarToSpawn; ++Index)
	{
		APlayableCharacter* Player = AlivePlayers[(FirstPlayerIndex + Index) % AlivePlayers.Num()];
		FPillarTarget& PillarTarget = PillarTargets.AddDefaulted_GetRef();
		PillarTarget.Target = Player;
		if (Index >= AlivePlayers.Num())
		{
			float const RandomAngle = Stream.FRandRange(0.f, 2.f * PI);
			float const RandomRadius = Stream.FRandRange(MinScatterRadius, MaxScatterRadius);
			PillarTarget.Offset = FVector2D(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle)) * RandomRadius;
		}
		PillarTarget.FallbackLocation = FVector2D(Player->GetActorLocation()) + PillarTarget.Offset;
	}
}

void UGeoSpawnPillarAbility::BeginPreLaunch()
{
	FillPillarTargets();

	TArray<TWeakObjectPtr<AActor>> PreLaunchedCuePlayers;
	for (FPillarTarget const& PillarTarget : PillarTargets)
	{
		if (IsValid(PillarTarget.Target.Get()) && !PreLaunchedCuePlayers.Contains(PillarTarget.Target))
		{
			PreLaunchedCuePlayers.Add(PillarTarget.Target);
			AddPreLaunchCue(GeoASLib::GetGeoAscFromActor(PillarTarget.Target.Get()));
		}
	}
}
void UGeoSpawnPillarAbility::LaunchPattern()
{
	if (PreLaunchDelay <= 0.f)
	{
		FillPillarTargets();
	}

	Super::LaunchPattern();
}


TInstancedStruct<FPatternData> UGeoSpawnPillarAbility::CreatePatternData() const
{
	FSpawnPillarPatternData PillarData;

	for (FPillarTarget const& PillarTarget : PillarTargets)
	{
		FVector2D ZoneLocation = PillarTarget.FallbackLocation;
		if (AActor const* Target = PillarTarget.Target.Get())
		{
			ZoneLocation = FVector2D(Target->GetActorLocation()) + PillarTarget.Offset;
		}
		PillarData.ZoneLocations.Add(ZoneLocation);
	}

	return TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(PillarData);
}
