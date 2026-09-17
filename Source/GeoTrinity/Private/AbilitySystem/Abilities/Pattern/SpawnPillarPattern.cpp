// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpawnPillarPattern.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Tool/UGeoGameplayLibrary.h"

USpawnPillarPattern::USpawnPillarPattern()
{
	bHasHazard = true;
}

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

bool USpawnPillarPattern::IsInHazard(AActor const* Target, FVector2D const Location, float /*SpentTime*/) const
{
	FGenericTeamId const SourceTeam = GeoASLib::GetTeamId(StoredPayload.SourceOwner);
	for (FVector2D const& ZoneLocation : PillarSpawnLocations)
	{
		if (GeoASLib::IsInCircle(Target, Location, ZoneLocation, SpawningZoneSize, ETargetOverlapMode::Automatic,
								 SourceTeam))
		{
			return true;
		}
	}
	return false;
}

TArray<TInstancedStruct<FEffectData>> const& USpawnPillarPattern::GetHazardEffects() const
{
	return PillarSpawnEffects;
}

void USpawnPillarPattern::EndPattern(bool const bForceStop)
{
	if (IsPatternActive() && !bForceStop && GeoLib::IsServer(GetWorld()))
	{
		for (FVector2D const& ZoneLocation : PillarSpawnLocations)
		{
			GeoASLib::FullySpawnDeployable(PillarClass, StoredPayload,
										   GeoASLib::GetEffectDataArray(StoredPayload.AbilityTag), PillarParams,
										   FTransform(FVector(ZoneLocation, ArbitraryCharacterZ)));
		}
	}

	Super::EndPattern(bForceStop);
}
