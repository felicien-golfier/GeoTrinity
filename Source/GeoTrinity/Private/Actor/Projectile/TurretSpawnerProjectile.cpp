// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Projectile/TurretSpawnerProjectile.h"

#include "Actor/Turret/GeoTurretBase.h"
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
	if (!IsValid(Owner))
	{
		ensureMsgf(false, TEXT("Owner is invalid!"));
		return;
	}

	if (!Owner->HasAuthority())
	{
		return;
	}

	FTransform const SpawnTransform = GetActorTransform();

	// Create turret
	checkf(TurretActorClass, TEXT("No Turret in the turret spawner!"));

	APawn* Pawn = Cast<APawn>(Owner);
	if (!IsValid(Pawn))
	{
		if (APlayerState const* PlayerController = Cast<APlayerState>(Owner))
		{
			Pawn = PlayerController->GetPawn();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No valid pawn to spawn turret!"));
			return;
		}
	}

	AGeoTurretBase* Turret = GetWorld()->SpawnActorDeferred<AGeoTurretBase>(
		TurretActorClass, SpawnTransform, Owner, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(Turret))
	{
		UE_LOG(LogTemp, Error, TEXT("No valid turret spawned!"));
		return;
	}

	FTurretData Data;
	Data.CharacterOwner = Owner;
	Data.Level = GetTurretLevel();
	Data.EffectDataArray = EffectDataArray;
	if (IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(Owner))
	{
		Data.TeamID = TeamInterface->GetGenericTeamId();
	}
	Turret->InitInteractableData(&Data);

	Turret->FinishSpawning(SpawnTransform);
}

bool ATurretSpawnerProjectile::IsValidOverlap(AActor const* OtherActor)
{
	return OtherActor != GetInstigator() && Super::IsValidOverlap(OtherActor);
}
