// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/DeployableSpawner/DeployableSpawnerProjectile.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
bool ADeployableSpawnerProjectile::IsValidOverlap(AActor const* OtherActor)
{
	return OtherActor != GetInstigator() && Super::IsValidOverlap(OtherActor);
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::EndProjectileLife()
{
	Super::EndProjectileLife();
	SpawnDeployableActor();
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::FillBaseData(FDeployableData& Data) const
{
	Data.Owner = Payload.Owner;
	Data.Instigator = Payload.Instigator;
	Data.Level = Payload.AbilityLevel;
	Data.Seed = Payload.Seed;
	Data.Params = Params;
	if (IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(Payload.Owner))
	{
		Data.TeamID = TeamInterface->GetGenericTeamId();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::InitDeployable(AGeoDeployableBase* Deployable) const
{
	FDeployableData Data;
	FillBaseData(Data);
	Data.EffectDataArray = EffectDataArray;
	Deployable->InitInteractable(&Data);
}

// ---------------------------------------------------------------------------------------------------------------------
void ADeployableSpawnerProjectile::SpawnDeployableActor()
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	auto Deployable = GeoASLib::StartSpawnDeployable(DeployableActorClass, Payload.Owner,
													 Cast<APawn>(Payload.Instigator), GetActorTransform());
	InitDeployable(Deployable); // TODO: Move code to ASLib
	GeoASLib::FinishSpawnDeployable(Deployable, GetActorTransform());
}
