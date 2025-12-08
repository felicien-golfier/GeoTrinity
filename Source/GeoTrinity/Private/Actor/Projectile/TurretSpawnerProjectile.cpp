// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Projectile/TurretSpawnerProjectile.h"

#include "Actor/Turret/GeoTurretBase.h"
#include "Characters/PlayableCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerState.h"

// ---------------------------------------------------------------------------------------------------------------------
ATurretSpawnerProjectile::ATurretSpawnerProjectile()
{
}

// ---------------------------------------------------------------------------------------------------------------------
float ATurretSpawnerProjectile::GetTurretLevel_Implementation() const
{
	return 1.f;
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
	checkf(IsValid(Owner), TEXT("Owner is invalid!"));
	if (!Owner->HasAuthority())
	{
		return;
	}

	const FTransform SpawnTransform = GetActorTransform();

	// Create turret
	checkf(TurretActorClass, TEXT("No Turret in the turret spawner!"));

	APawn* Pawn = Cast<APawn>(Owner);
	if (!IsValid(Pawn))
	{
		if (const APlayerState* PlayerController = Cast<APlayerState>(Owner))
		{
			Pawn = PlayerController->GetPawn();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No valid pawn to spawn turret!"));
			return;
		}
	}

	AGeoTurretBase* Turret = GetWorld()->SpawnActorDeferred<AGeoTurretBase>(TurretActorClass, SpawnTransform, Owner,
		Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!IsValid(Turret))
	{
		UE_LOG(LogTemp, Error, TEXT("No valid turret spawned!"));
		return;
	}

	TurretInitData Data;
	Data.CharacterOwner = Owner;
	Data.TurretLevel = GetTurretLevel();
	Data.BulletsDamageEffectParams = DamageEffectParams;
	Turret->InitTurretData(Data);

	Turret->FinishSpawning(SpawnTransform);
}

bool ATurretSpawnerProjectile::IsValidOverlap(const AActor* OtherActor)
{
	bool bHasValidOverlap = Super::IsValidOverlap(OtherActor);
	if (!bHasValidOverlap)
	{
		return false;
	}

	return OtherActor != GetInstigator();
}