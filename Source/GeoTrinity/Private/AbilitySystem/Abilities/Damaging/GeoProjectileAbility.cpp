// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Data/EffectData.h" //Necessary to copy array of EffectData.
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "System/GeoActorPoolingSubsystem.h"

void UGeoProjectileAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
											const FGameplayAbilityActorInfo* ActorInfo,
											const FGameplayAbilityActivationInfo ActivationInfo,
											const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Build payload from avatar transform
	AActor* Instigator = GetAvatarActorFromActorInfo();
	StoredPayload = CreateAbilityPayload(Instigator->GetTransform(), GetOwningActorFromActorInfo(), Instigator);

	// Extract orientation from TriggerEventData if available (sent via event-based activation)
	// Both client and server receive the same event data in a single RPC

	ensureMsgf(TriggerEventData && TriggerEventData->TargetData.Num() > 0, TEXT("No TargetData in TriggerEventData!"));

	if (const FGeoAbilityTargetData_Orientation* OrientationData =
			static_cast<const FGeoAbilityTargetData_Orientation*>(TriggerEventData->TargetData.Get(0)))
	{
		StoredPayload.Origin = OrientationData->Origin;
		StoredPayload.Yaw = OrientationData->Yaw;
	}

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

void UGeoProjectileAbility::SpawnProjectileUsingLocation(const FVector& projectileTargetLocation)
{
	const AActor* Actor = GetAvatarActorFromActorInfo();
	checkf(IsValid(Actor), TEXT("Avatar Actor from actor info is invalid!"));

	FTransform SpawnTransform{(projectileTargetLocation - Actor->GetActorLocation()).Rotation().Quaternion(),
							  Actor->GetActorLocation()};
	// Optionally spawn from a socket named by the current montage section index
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const USkeletalMeshComponent* Mesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
	if (bUseSocketFromSectionIndex && ASC && Mesh)
	{
		const FName CurrentSection = ASC->GetCurrentMontageSectionName();
		const FString IndexString = CurrentSection.ToString().Right(1);
		const FName SocketName(*IndexString);

		if (IndexString.IsNumeric() && Mesh->DoesSocketExist(SocketName))
		{
			SpawnTransform.SetLocation(Mesh->GetSocketLocation(SocketName));
		}
	}

	SpawnProjectile(SpawnTransform);
}

void UGeoProjectileAbility::SpawnProjectile(const FTransform SpawnTransform) const
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
		UE_LOG(LogTemp, Error, TEXT("[Projectile] FAILED: RequestActor returned nullptr for %s"),
			   *ProjectileClass->GetName());
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
			return {FRotator(0.f, StoredPayload.Yaw, 0.f).Vector()};
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
