// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Tool/GameplayLibrary.h"

void UGeoProjectileAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											FGameplayAbilityActorInfo const* ActorInfo,
											FGameplayAbilityActivationInfo const ActivationInfo,
											FGameplayEventData const* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Build payload from avatar transform
	AActor* Instigator = GetAvatarActorFromActorInfo();
	if (FGeoAbilityTargetData const* TargetData =
			static_cast<FGeoAbilityTargetData const*>(TriggerEventData->TargetData.Get(0)))
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, TargetData->Origin,
											 TargetData->Yaw, TargetData->ServerSpawnTime, TargetData->Seed);
	}
	else
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, Instigator->GetTransform());
	}

	// Extract orientation from TriggerEventData if available (sent via event-based activation)
	// Both client and server receive the same event data in a single RPC

	ensureMsgf(TriggerEventData && TriggerEventData->TargetData.Num() > 0, TEXT("No TargetData in TriggerEventData!"));


	// Proceed with montage or spawn
	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (IsValid(AnimInstance) && AnimMontage)
	{
		HandleAnimationMontage(AnimInstance, ActivationInfo);
	}
	else
	{
		AnimTrigger();
	}
}


void UGeoProjectileAbility::AnimTrigger()
{
	Super::AnimTrigger();
	SpawnProjectilesUsingTarget();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

void UGeoProjectileAbility::SpawnProjectileUsingDirection(FVector const& Direction)
{
	AActor const* Instigator = GetAvatarActorFromActorInfo();
	checkf(IsValid(Instigator), TEXT("Avatar Actor from actor info is invalid!"));

	FTransform SpawnTransform{Direction.Rotation().Quaternion(), Instigator->GetActorLocation()};

	// Optionally spawn from a socket named by the current montage section index
	if (bUseSocketFromSectionIndex)
	{
		UAbilitySystemComponent const* ASC = GetAbilitySystemComponentFromActorInfo();
		USkeletalMeshComponent const* Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>();
		if (ASC && Mesh)
		{
			FName const CurrentSection = ASC->GetCurrentMontageSectionName();
			FString const IndexString = CurrentSection.ToString().Right(1);
			FName const SocketName(*IndexString);

			if (IndexString.IsNumeric() && Mesh->DoesSocketExist(SocketName))
			{
				SpawnTransform.SetLocation(Mesh->GetSocketLocation(SocketName));
			}
		}
	}

	SpawnProjectile(SpawnTransform);
}

void UGeoProjectileAbility::SpawnProjectilesUsingTarget()
{
	TArray<FVector> const Directions =
		GameplayLibrary::GetTargetDirections(GetWorld(), Target, StoredPayload.Yaw, FVector(StoredPayload.Origin, 0.f));
	for (FVector const& Direction : Directions)
	{
		SpawnProjectileUsingDirection(Direction);
	}
}

void UGeoProjectileAbility::SpawnProjectile(FTransform const SpawnTransform) const
{
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));
	GameplayLibrary::SpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, StoredPayload, GetEffectDataArray());
}
