// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticProjectileAbility.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/GameplayLibrary.h"

bool UGeoAutomaticProjectileAbility::ExecuteShot_Implementation()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[GeoAutomaticProjectileAbility] No ProjectileClass set!"));
		return false;
	}

	FVector const Origin{StoredPayload.Origin, 0.f};
	float const ProjectileYaw =
		GameplayLibrary::GetYawWithNetworkDelay(GetAvatarActorFromActorInfo(), CachedNetworkDelay);
	TArray<FVector> const Directions = GameplayLibrary::GetTargetDirections(GetWorld(), Target, ProjectileYaw, Origin);

	bool bAnySpawned = false;
	for (FVector const& Direction : Directions)
	{
		FTransform const SpawnTransform{Direction.Rotation().Quaternion(), Origin};
		if (GameplayLibrary::SpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload,
											 GetEffectDataArray()))
		{
			bAnySpawned = true;
		}
	}

	return bAnySpawned;
}
