// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"

#include <string>

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Data/EffectData.h" //Necessary to copy array of EffectData.
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "GeoTrinity/GeoTrinity.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "Tool/GameplayLibrary.h"

void UGeoProjectileAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
											const FGameplayAbilityActorInfo* ActorInfo,
											const FGameplayAbilityActivationInfo ActivationInfo,
											const FGameplayEventData* TriggerEventData)
{
	AActor* Owner = GetOwningActorFromActorInfo();
	StoredPayload = CreateAbilityPayload(Owner->GetTransform(), Owner, GetAvatarActorFromActorInfo());
	// TODO: Use (ablility?) context handle to send payload
	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimMontage && AnimInstance)
	{
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
			return;
		}

		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (!AnimInstance->Montage_IsPlaying(AnimMontage))
		{
			ASC->PlayMontage(this, ActivationInfo, AnimMontage, 1.f);
		}
		else
		{
			const FName CurrentSection = ASC->GetCurrentMontageSectionName();
			const FName FireSection = GameplayLibrary::SectionFireName;
			FName SectionToJumpTo;
			if (CurrentSection == GameplayLibrary::SectionStartName)
			{
				SectionToJumpTo = AnimMontage->GetSectionName(ASC->GetCurrentMontageSectionID() + 1);
			}
			else
			{
				const FString IndexChar = CurrentSection.ToString().Right(1);
				if (!IndexChar.IsNumeric())
				{
					// Case where we do have a single Fire Section.
					SectionToJumpTo = FireSection;
				}
				else
				{
					int i = FCString::Atoi(*IndexChar);
					FString NextFire = FireSection.ToString();
					NextFire.AppendInt(++i);
					if (!AnimMontage->IsValidSectionName(FName(NextFire)))
					{
						// Cycle back to 1.
						NextFire = FireSection.ToString();
						NextFire.AppendInt(1);
					}

					SectionToJumpTo = FName(NextFire);
				}
			}

			if (!AnimMontage->IsValidSectionName(SectionToJumpTo))
			{
				UE_LOG(LogGeoASC, Error, TEXT("Section %s doesn't exist ! Fallback to Start."),
					   *SectionToJumpTo.ToString());
				SectionToJumpTo = GameplayLibrary::SectionStartName;
			}

			ASC->CurrentMontageJumpToSection(SectionToJumpTo);
		}

		const float SectionTimeRemaining = ASC->GetCurrentMontageSectionTimeLeft();
		constexpr float FrameSecurity = 0.016f;
		if (SectionTimeRemaining > FrameSecurity)
		{
			GetWorld()->GetTimerManager().SetTimer(StartSectionTimerHandle, this,
												   &UGeoProjectileAbility::OnMontageSectionStartEnded,
												   SectionTimeRemaining - FrameSecurity);
		}
		else
		{
			OnMontageSectionStartEnded();
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGeoProjectileAbility::OnMontageSectionStartEnded()
{
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
