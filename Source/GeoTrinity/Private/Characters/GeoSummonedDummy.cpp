// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/GeoSummonedDummy.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoSummonedDummy::BeginPlay()
{
	Super::BeginPlay();

	if (GeoLib::IsServer(this))
	{
		TSubclassOf<AGeoEffectZone> const ZoneClass =
			GetDefault<UGameDataSettings>()->DefaultZoneClass.LoadSynchronous();
		if (ensureMsgf(ZoneClass, TEXT("%s: no DefaultZoneClass in Game Data Settings"), *GetName()))
		{
			FDeployableData ZoneData;
			ZoneData.Owner = this;
			ZoneData.Instigator = this;
			ZoneData.TeamID = GetGenericTeamId();
			ZoneData.EffectDataArray = ZoneEffects;
			ZoneData.Params = ZoneParams;
			FVector Location = GetActorLocation();
			Location.Z = 0.f;
			GeoASLib::FullySpawnDeployable(ZoneClass, ZoneData, FTransform(Location));
		}
	}
}

void AGeoSummonedDummy::DeathLogic()
{
	Super::DeathLogic();

	AGeoArena* const MyArena = Arena.Get();
	if (GeoLib::IsServer(this) && ensureMsgf(MyArena, TEXT("%s: summoned outside an arena"), *GetName())
		&& ensureMsgf(AllyClass, TEXT("%s: no AllyClass set"), *GetName()))
	{
		MyArena->SpawnAdd(AllyClass, GetActorTransform(), &AllySetup);
	}
}
