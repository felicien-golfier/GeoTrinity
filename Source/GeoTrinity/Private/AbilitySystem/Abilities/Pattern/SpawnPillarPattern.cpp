// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

void USpawnPillarPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	// TODO: why not having deployables in the pooling system. Need to set it up properly
	//  UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(PillarClass, 10);

	ensureMsgf(IsValid(PillarClass), TEXT("%hs: PillarClass is not set on %s"), __FUNCTION__, *GetName());
}

FGameplayCueParameters USpawnPillarPattern::FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams = Super::FillCueParam(Cue, Payload);
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
void USpawnPillarPattern::ExecuteGameplayCue(FGeoCueParam const& Cue)
{
	// Local cue: run on every rendering machine incl. the listen-server host; skip only the dedicated server.
	if (Cue.IsValid() && !GeoLib::IsDedicatedServer(GetWorld()))
	{
		UGeoAbilitySystemComponent* AvatarASC = GeoASLib::GetGeoAscFromActor(StoredPayload.SourceAvatar);
		if (ensureMsgf(IsValid(AvatarASC), TEXT("Pattern source avatar %s has no ASC !"),
					   *GetNameSafe(StoredPayload.SourceAvatar)))
		{
			for (FVector2D const& Location : PillarSpawnLocations)
			{
				FGameplayCueParameters CueParams = FillCueParam(Cue, StoredPayload);
				CueParams.Location = FVector(Location, ArbitraryCharacterZ);
				GeoASLib::ExecuteGeoCue(AvatarASC, Cue, CueParams, true);
			}
		}
	}
}

void USpawnPillarPattern::StartPattern()
{
	Super::StartPattern();
	ZoneExpiry.Start(StoredPayload.ServerSpawnTime + StartDelay, /*bSeenThroughReplication*/ false);

	if (!GeoLib::IsServer(GetWorld()))
	{
		EndPattern();
	}
}

void USpawnPillarPattern::TickPattern(float const ServerTime, float /*SpentTime*/)
{
	FGenericTeamId const SourceTeam = GeoASLib::GetTeamId(StoredPayload.SourceOwner);
	TSet<AActor*> const ActorsReachingExpiry = ZoneExpiry.JudgeActorsReachingEvent(
		GeoASLib::GetInteractableActors(this, SourceTeam, TeamAttitudeMask::HostileOrNeutral, true));

	UGeoAbilitySystemComponent* const AvatarASC = GeoASLib::GetGeoAscFromActor(StoredPayload.SourceAvatar);
	if (AvatarASC && PillarSpawnEffects.Num() > 0 && ActorsReachingExpiry.Num() > 0)
	{
		for (FVector2D const& ZoneLocation : PillarSpawnLocations)
		{
			for (AActor* TargetActor : GeoASLib::GetInteractableActors(
					 this, SourceTeam, TeamAttitudeMask::HostileOrNeutral, true, ZoneLocation, SpawningZoneSize,
					 [&ActorsReachingExpiry](AActor* Actor)
					 {
						 return ActorsReachingExpiry.Contains(Actor);
					 }))
			{
				if (IsValid(TargetActor) && !TargetActor->IsActorBeingDestroyed())
				{
					if (UGeoAbilitySystemComponent* TargetASC = GeoASLib::GetGeoAscFromActor(TargetActor))
					{
						UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(
							PillarSpawnEffects, AvatarASC, TargetASC, StoredPayload.AbilityLevel, StoredPayload.Seed,
							StoredPayload.AbilityTag);
						UGeoAbilitySystemLibrary::NotifyAbilityHit(StoredPayload, TargetActor);
					}
				}

				if (!bPatternIsActive) // Cuz previous effect can kill the last char and so delete the boss.
				{
					return;
				}
			}
		}
	}

	if (ZoneExpiry.IsOver(ServerTime))
	{

		for (FVector2D const& ZoneLocation : PillarSpawnLocations)
		{
			GeoASLib::FullySpawnDeployable(PillarClass, StoredPayload,
										   GeoASLib::GetEffectDataArray(StoredPayload.AbilityTag), PillarParams,
										   FTransform(FVector(ZoneLocation, ArbitraryCharacterZ)));
		}

		EndPattern();
	}
}
