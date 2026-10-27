// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoSpawnZoneAction.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Characters/PlayableCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Engine/World.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

void UGeoSpawnZoneAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	OnCancel_Implementation();
	GetEncounter()->HoldUntilStepLeft(*this);
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
		Location.Z = ArbitraryCharacterZ;
		FDeployableData Data;
		Data.TeamID = FGenericTeamId(static_cast<uint8>(Team));
		Data.EffectDataArray = EffectDataArray;
		Data.Params = Params;
		Zone = Cast<AGeoEffectZone>(GeoASLib::FullySpawnDeployable(ZoneClass, Data, FTransform(Location), GetWorld()));
		if (Zone.IsValid() && EnteredEvent.IsValid())
		{
			Zone->OnActorBeginOverlap.AddDynamic(this, &UGeoSpawnZoneAction::OnZoneBeginOverlap);
			TArray<AActor*> Inside;
			Zone->GetOverlappingActors(Inside, APlayableCharacter::StaticClass());
			for (AActor* const Player : Inside)
			{
				RaiseEntered(*Player);
			}
		}
	}
}

void UGeoSpawnZoneAction::OnCancel_Implementation()
{
	if (Zone.IsValid())
	{
		Zone->OnActorBeginOverlap.RemoveDynamic(this, &UGeoSpawnZoneAction::OnZoneBeginOverlap);
		Zone->Recall();
		Zone.Reset();
	}
}

void UGeoSpawnZoneAction::OnZoneBeginOverlap(AActor* /*OverlappedActor*/, AActor* OtherActor)
{
	if (Cast<APlayableCharacter>(OtherActor))
	{
		RaiseEntered(*OtherActor);
	}
}

void UGeoSpawnZoneAction::RaiseEntered(AActor& Player) const
{
	FGeoEncounterPayload Payload;
	Payload.Actor = &Player;
	GetWorld()->GetSubsystem<UGeoEncounterSubsystem>()->Raise(EnteredEvent, Payload);
}
