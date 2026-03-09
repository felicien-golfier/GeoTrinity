// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/TurretSpawnerProjectile.h"

#include "Actor/Turret/GeoTurretBase.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "GameFramework/PlayerState.h"

// ---------------------------------------------------------------------------------------------------------------------
ATurretSpawnerProjectile::ATurretSpawnerProjectile()
{
}

// ---------------------------------------------------------------------------------------------------------------------
float ATurretSpawnerProjectile::GetTurretLevel_Implementation() const
{
	return Payload.AbilityLevel;
}

// ---------------------------------------------------------------------------------------------------------------------
void ATurretSpawnerProjectile::EndProjectileLife()
{
	Super::EndProjectileLife();
	SpawnTurretActor();
}

// ---------------------------------------------------------------------------------------------------------------------
void ATurretSpawnerProjectile::SpawnTurretActor() const
{
	AActor* PayloadOwner = Payload.Owner;
	ensureMsgf(IsValid(PayloadOwner), TEXT("TurretSpawnerProjectile: Payload.Owner is invalid!"));
	if (!IsValid(PayloadOwner))
	{
		return;
	}

	if (!PayloadOwner->HasAuthority())
	{
		return;
	}

	FTransform const SpawnTransform = GetActorTransform();

	checkf(TurretActorClass, TEXT("No Turret in the turret spawner!"));

	APawn* Pawn = Cast<APawn>(PayloadOwner);
	if (!IsValid(Pawn))
	{
		if (APlayerState const* PlayerState = Cast<APlayerState>(PayloadOwner))
		{
			Pawn = PlayerState->GetPawn();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("TurretSpawnerProjectile: No valid pawn to spawn turret!"));
			return;
		}
	}

	AGeoTurretBase* Turret = GetWorld()->SpawnActorDeferred<AGeoTurretBase>(
		TurretActorClass, SpawnTransform, PayloadOwner, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(Turret))
	{
		UE_LOG(LogTemp, Error, TEXT("TurretSpawnerProjectile: No valid turret spawned!"));
		return;
	}

	FTurretData Data;
	Data.CharacterOwner = PayloadOwner;
	Data.Level = GetTurretLevel();
	Data.EffectDataArray = EffectDataArray;
	if (IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(PayloadOwner))
	{
		Data.TeamID = TeamInterface->GetGenericTeamId();
	}
	Turret->InitInteractableData(&Data);

	Turret->FinishSpawning(SpawnTransform);

	if (UGeoDeployableManagerComponent* DeployableManager = Pawn->GetComponentByClass<UGeoDeployableManagerComponent>())
	{
		DeployableManager->RegisterDeployable(Turret);
	}
}

bool ATurretSpawnerProjectile::IsValidOverlap(AActor const* OtherActor)
{
	return OtherActor != GetInstigator() && Super::IsValidOverlap(OtherActor);
}
