// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Projectile/TurretSpawnerProjectile.h"

#include "Actor/Turret/GeoTurretBase.h"



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
	SpawnTurretActor();
	
	Super::EndProjectileLife();
}

// ---------------------------------------------------------------------------------------------------------------------
void ATurretSpawnerProjectile::SpawnTurretActor() const
{
	AActor* pAvatarOwner = GetOwner();
	const bool bIsServer = pAvatarOwner ? pAvatarOwner->HasAuthority() : false;
	if (!bIsServer)
		return;
	
	FTransform const spawnTransform = GetActorTransform();
	
	// Create turret
	checkf(TurretActorClass, TEXT("No Turret in the turret spawner!"));
	
	AGeoTurretBase* pTurret = GetWorld()->SpawnActorDeferred<AGeoTurretBase>(
		TurretActorClass, 
		spawnTransform, 
		pAvatarOwner, 
		Cast<APawn>(pAvatarOwner), 
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (!pTurret)
		return;
	
	TurretInitData data;
	data.CharacterOwner = pAvatarOwner;
	data.TurretLevel = GetTurretLevel();
	data.BulletsDamageEffectParams = DamageEffectParams;
	pTurret->InitTurretData(data);

	pTurret->FinishSpawning(spawnTransform);
}

