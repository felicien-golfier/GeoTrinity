// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "Actor/Projectile/GeoProjectile.h"

void UGeoProjectileAbility::SpawnProjectileUsingLocation(const FVector& projectileTargetLocation)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));
	if (!Actor->HasAuthority())
	{
		return;
	}

	SpawnProjectile((projectileTargetLocation - Actor->GetActorLocation()).Rotation());
}

void UGeoProjectileAbility::SpawnProjectile(const FRotator& DirectionRotator)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));
	if (!Actor->HasAuthority())
	{
		return;
	}

	const FTransform spawnTransform{DirectionRotator.Quaternion(), Actor->GetActorLocation()};

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

void UGeoProjectileAbility::SpawnProjectilesUsingTarget()
{
	const TArray<FVector> Locations = GetTargetLocations();
	for (FVector const& Location : Locations)
	{
		SpawnProjectileUsingLocation(Location);
	}
}

TArray<FVector> UGeoProjectileAbility::GetTargetLocations()
{
	switch (Target)
	{
		case ETarget::Forward:
		{
			AActor* Actor = GetAvatarActorFromActorInfo();
			return {Actor->GetActorForwardVector() + Actor->GetActorLocation()};
		}

		case ETarget::AllPlayers:
		{
			TArray<FVector> Locations;
			for (auto PlayerControllerIt = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIt;
				++PlayerControllerIt)
			{
				if (APlayerController* PlayerController = PlayerControllerIt->Get())
				{
					Locations.Add(PlayerController->GetPawn()->GetActorLocation());
				}
			}
			return Locations;
		}

		default:
		{
			return {};
		}
	}
}