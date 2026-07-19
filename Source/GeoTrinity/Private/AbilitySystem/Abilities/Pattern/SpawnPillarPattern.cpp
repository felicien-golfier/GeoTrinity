// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void USpawnPillarPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	// TODO: why not having deployables in the pooling system. Need to set it up properly
	//  UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(PillarClass, 10);
	UGeoDeployableManagerComponent* DeployablesManagerComp =
		Owner.GetComponentByClass<UGeoDeployableManagerComponent>();
	if (DeployablesManagerComp)
	{
		DeployablesManagerComp->SetDeployableInfinitCount(PillarClass);
	}

	if (!IsValid(PillarClass))
	{
		ensureMsgf(false, TEXT("PillarClass null, please fill the class in the FatalZonePattern"));
	}
}

FGameplayCueParameters USpawnPillarPattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Payload);
	CueParams.RawMagnitude = SpawningZoneSize;
	return CueParams;
}

void USpawnPillarPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	PillarSpawnLocations.Empty();

	FSpawnPillarPatternData const* PillarData = PatternData.GetPtr<FSpawnPillarPatternData>();
	if (!ensureMsgf(PillarData,
					TEXT("SpawnPillarPattern: PatternData is not an FSpawnPillarPatternData — launch this "
						 "pattern from USpawnPillarAbility")))
	{
		Super::InitPattern(Payload, PatternData);
		return;
	}

	for (FVector2D const& ZoneLocation : PillarData->ZoneLocations)
	{
		PillarSpawnLocations.Add(ZoneLocation);
	}
	Super::InitPattern(Payload, PatternData);

	ExecuteGameplayCue(DirectionCue); // Call after super to have Storedpayload
}
void USpawnPillarPattern::ExecuteGameplayCue(FGameplayTag GameplayCueTag)
{
	// Local cue: run on every rendering machine incl. the listen-server host; skip only the dedicated server.
	if (GameplayCueTag.IsValid() && !GeoLib::IsDedicatedServer(GetWorld()))
	{
		UGeoAbilitySystemComponent* InstigatorASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);
		if (ensureMsgf(IsValid(InstigatorASC), TEXT("Pattern Instigator %s has no ASC !"),
					   *StoredPayload.Instigator->GetName()))
		{
			for (FVector2D const& Location : PillarSpawnLocations)
			{
				FGameplayCueParameters CueParams = FillCueParam(StoredPayload);
				CueParams.Location = FVector(Location, ArbitraryCharacterZ);
				GeoASLib::ExecuteLocalGameplayCue(InstigatorASC, GameplayCueTag, CueParams);
			}
		}
	}
}

void USpawnPillarPattern::StartPattern()
{
	Super::StartPattern();
	UGeoAbilitySystemComponent* InstigatorAsc = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);

	if (UGeoGameplayLibrary::IsServer(GetWorld()))
	{
		for (FVector2D const& Location : PillarSpawnLocations)
		{
			SpawnPillarAtLocation(Location, InstigatorAsc);
			if (!bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
			{
				return;
			}
		}
	}

	EndPattern();
}

void USpawnPillarPattern::SpawnPillarAtLocation(FVector2D const& ZoneLocation,
												UGeoAbilitySystemComponent* InstigatorAsc) const
{
	if (InstigatorAsc && PillarSpawnEffects.Num() > 0)
	{
		for (AActor* TargetActor :
			 GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(StoredPayload.Owner),
											 TeamAttitudeMask::HostileOrNeutral, true, ZoneLocation, SpawningZoneSize))
		{
			if (!bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
			{
				return;
			}

			if (IsValid(TargetActor) && !TargetActor->IsActorBeingDestroyed())
			{
				if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(TargetActor))
				{
					UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(PillarSpawnEffects, InstigatorAsc, TargetASC,
																		StoredPayload.AbilityLevel, StoredPayload.Seed,
																		StoredPayload.AbilityTag);
				}
			}
		}
	}

	if (bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
	{
		GeoASLib::FullySpawnDeployable(PillarClass, StoredPayload,
									   GeoASLib::GetEffectDataArray(StoredPayload.AbilityTag), PillarParams,
									   FTransform(FVector(ZoneLocation, ArbitraryCharacterZ)));
	}
}
