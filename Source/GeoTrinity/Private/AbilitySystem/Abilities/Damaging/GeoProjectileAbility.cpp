// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Data/EffectData.h" //Necessary to copy array of EffectData.
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

void UGeoProjectileAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
											const FGameplayAbilityActorInfo* ActorInfo,
											const FGameplayAbilityActivationInfo ActivationInfo,
											const FGameplayEventData* TriggerEventData)
{
	AActor* Owner = GetOwningActorFromActorInfo();
	StoredPayload = CreateAbilityPayload(Owner->GetTransform(), Owner, GetAvatarActorFromActorInfo());
	// TODO: Use (ablility?) context handle to send payload

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (IsValid(AnimInstance) && AnimMontage)
	{
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
			return;
		}
		HandleAnimationMontage(AnimInstance, ActivationInfo);
	}
	else
	{
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
			return;
		}
		AnimTrigger();
	}
}


void UGeoProjectileAbility::AnimTrigger()
{
	Super::AnimTrigger();
	SpawnProjectilesUsingTarget();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

void UGeoProjectileAbility::SpawnProjectileUsingLocation(const FVector& projectileTargetLocation)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));

	FTransform SpawnTransform{(projectileTargetLocation - Actor->GetActorLocation()).Rotation().Quaternion(),
							  Actor->GetActorLocation()};
	SpawnProjectile(SpawnTransform);
}

void UGeoProjectileAbility::SpawnProjectile(const FTransform& SpawnTransform) const
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));

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
	GeoProjectile->Payload = StoredPayload;
	GeoProjectile->EffectDataArray = GetEffectDataArray();

	GeoProjectile->Init(); // Equivalent to the DeferredSpawn
}

void UGeoProjectileAbility::SpawnProjectilesUsingTarget()
{
	const TArray<FVector> Locations = GetTargetLocations();
	for (const FVector& Location : Locations)
	{
		SpawnProjectileUsingLocation(Location);
	}
}

TArray<FVector> UGeoProjectileAbility::GetTargetLocations() const
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
