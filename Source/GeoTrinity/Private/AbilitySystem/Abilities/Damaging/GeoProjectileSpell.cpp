// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileSpell.h"

#include "Actor/Projectile/GeoProjectile.h"

void UGeoProjectileSpell::SpawnProjectile(const FVector& projectileTargetLocation)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));
	if (!Actor->HasAuthority())
	{
		return;
	}

	FRotator rotation = (projectileTargetLocation - Actor->GetActorLocation()).Rotation();

	const FTransform spawnTransform{rotation.Quaternion(), Actor->GetActorLocation()};

	// Create projectile
	AActor* ProjectileOwner = GetOwningActorFromActorInfo();
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));
	AGeoProjectile* GeoProjectile = GetWorld()->SpawnActorDeferred<AGeoProjectile>(ProjectileClass, spawnTransform,
		ProjectileOwner, Cast<APawn>(ProjectileOwner), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!GeoProjectile)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid Projectile spawned !"));
		return;
	}

	// Append GAS data
	checkf(DamageEffectClass, TEXT("No DamageEffectClass in the projectile spell!"));

	GeoProjectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults(nullptr);

	GeoProjectile->FinishSpawning(spawnTransform);
}