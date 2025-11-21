// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Damaging/GeoProjectileSpell.h"

#include "Actor/Projectile/GeoProjectile.h"

void UGeoProjectileSpell::SpawnProjectile(FVector const& projectileTargetLocation)
{
	AActor* pAvatar = GetAvatarActorFromActorInfo();
	const bool bIsServer = pAvatar ? pAvatar->HasAuthority() : false;
	if (!bIsServer)
		return;

	FRotator rotation = (projectileTargetLocation - pAvatar->GetActorLocation()).Rotation();

	FTransform const spawnTransform{rotation.Quaternion(), pAvatar->GetActorLocation()};
	
	// Create projectile
	AActor* pOwner = GetOwningActorFromActorInfo();
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));
	AGeoProjectile* pProjectile = GetWorld()->SpawnActorDeferred<AGeoProjectile>(ProjectileClass, spawnTransform, pOwner, 
		Cast<APawn>(pOwner), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!pProjectile)
		return;
	
	// Append GAS data
	checkf(DamageEffectClass, TEXT("No DamageEffectClass in the projectile spell!"));

	pProjectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults(nullptr);
	
	pProjectile->FinishSpawning(spawnTransform);
}