// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoSpawnZoneAction.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Engine/World.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoSpawnZoneAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	TArray<AActor*> const SpawnPoints = GeoLib::GetTargetPoints(this, PurposeTag, ArenaTag);
	TSubclassOf<AGeoEffectZone> const ZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass.LoadSynchronous();
	if (SpawnPoints.IsEmpty()
		|| !ensureMsgf(ZoneClass, TEXT("%s: no DefaultZoneClass in Game Data Settings"), *GetName()))
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no %s point tagged %s to spawn a zone on"), *GetName(),
			   *PurposeTag.ToString(), *ArenaTag.ToString());
	}
	else
	{
		FVector Location = SpawnPoints[0]->GetActorLocation();
		Location.Z = 0.f;
		FDeployableData Data;
		Data.TeamID = FGenericTeamId(static_cast<uint8>(Team));
		Data.EffectDataArray = EffectDataArray;
		Data.Params = Params;
		Zone = Cast<AGeoEffectZone>(GeoASLib::FullySpawnDeployable(ZoneClass, Data, FTransform(Location), GetWorld()));
	}
}

void UGeoSpawnZoneAction::OnCancel_Implementation()
{
	if (Zone.IsValid())
	{
		Zone->Recall();
	}
}
