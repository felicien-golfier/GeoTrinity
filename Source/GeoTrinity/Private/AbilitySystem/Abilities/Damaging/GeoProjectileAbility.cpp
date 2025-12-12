// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

void UGeoProjectileAbility::SpawnProjectileUsingLocation(const FVector& projectileTargetLocation)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));

	SpawnProjectile((projectileTargetLocation - Actor->GetActorLocation()).Rotation());
}

void UGeoProjectileAbility::SpawnProjectile(const FRotator& DirectionRotator)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));

	const FTransform SpawnTransform{DirectionRotator.Quaternion(), Actor->GetActorLocation()};

	// Create projectile
	AActor* ProjectileOwner = GetOwningActorFromActorInfo();
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));

	AGeoProjectile* GeoProjectile =
		UGeoActorPoolingSubsystem::Get(GetWorld())
			->RequestActor(ProjectileClass, SpawnTransform, ProjectileOwner, Cast<APawn>(ProjectileOwner), false);

	if (!GeoProjectile)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid Projectile pooled (ahah) !"));
		return;
	}

	// Append GAS data
	checkf(DamageEffectClass, TEXT("No DamageEffectClass in the projectile spell!"));

	GeoProjectile->EffectDataArray = UGeoAbilitySystemLibrary::GetEffectDataArray(EffectDataAsset);

	GeoProjectile->Init();   // Equivalent to the DeferredSpawn
}

void UGeoProjectileAbility::SpawnProjectilesUsingTarget()
{
	const TArray<FVector> Locations = GetTargetLocations();
	for (const FVector& Location : Locations)
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