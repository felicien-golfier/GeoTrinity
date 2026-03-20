// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Pickup/GeoBuffPickup.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoReloadAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo,
										FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	ScheduleFireTrigger(ActivationInfo, ActorInfo->GetAnimInstance());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoReloadAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	if (!GetCurrentActorInfo()->IsNetAuthority())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	float const CurrentAmmo = ASC->GetNumericAttribute(UCharacterAttributeSet::GetAmmoAttribute());
	float const MaxAmmo = ASC->GetNumericAttribute(UCharacterAttributeSet::GetMaxAmmoAttribute());
	float const MissingAmmo = MaxAmmo - CurrentAmmo;
	float const PowerScale = MaxAmmo > 0.f ? MissingAmmo / MaxAmmo : 0.f;

	ensureMsgf(AmmoRestoreEffect, TEXT("GeoTriangleReloadAbility: AmmoRestoreEffect is not set!"));
	if (AmmoRestoreEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
		FGameplayEffectSpecHandle const Spec = ASC->MakeOutgoingSpec(AmmoRestoreEffect, GetAbilityLevel(), Context);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	ensureMsgf(BuffPickupClass, TEXT("GeoTriangleReloadAbility: BuffPickupClass is not set!"));
	TArray<TInstancedStruct<FEffectData>> BuffEffects = GetEffectDataArray();
	ensureMsgf(!BuffEffects.IsEmpty(), TEXT("GeoTriangleReloadAbility: "));
	if (BuffPickupClass && !BuffEffects.IsEmpty())
	{
		int32 const RandomIndex = AbilityTargetData.Seed % BuffEffects.Num();

		AActor* Avatar = GetAvatarActorFromActorInfo();
		float const RandomAngle = FMath::RandRange(0.f, 2.f * PI);
		float const RandomRadius = FMath::RandRange(MinSpawnRadius, MaxSpawnRadius);
		FVector const SpawnOffset{FMath::Cos(RandomAngle) * RandomRadius, FMath::Sin(RandomAngle) * RandomRadius, 0.f};
		FVector const AvatarLocation = Avatar->GetActorLocation();
		FTransform const SpawnTransform{AvatarLocation};

		AGeoBuffPickup* Pickup =
			GetWorld()->SpawnActorDeferred<AGeoBuffPickup>(BuffPickupClass, SpawnTransform, Avatar, Cast<APawn>(Avatar),
														   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		ensureMsgf(IsValid(Pickup), TEXT("GeoTriangleReloadAbility: Failed to spawn AGeoBuffPickup!"));
		if (IsValid(Pickup))
		{
			FBuffPickupData PickupData;
			PickupData.CharacterOwner = Avatar;
			PickupData.Level = GetAbilityLevel();
			PickupData.EffectDataArray = {BuffEffects[RandomIndex]};
			PickupData.PowerScale = PowerScale;
			PickupData.MeshIndex = RandomIndex;
			PickupData.TargetLocation = AvatarLocation + SpawnOffset;
			if (IGenericTeamAgentInterface const* TeamInterface = Cast<IGenericTeamAgentInterface>(Avatar))
			{
				PickupData.TeamID = TeamInterface->GetGenericTeamId();
			}

			Pickup->InitInteractableData(&PickupData);
			Pickup->FinishSpawning(SpawnTransform);
		}
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}
