// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoTriangleReloadAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Pickup/GeoBuffPickup.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriangleReloadAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
												FGameplayAbilityActorInfo const* ActorInfo,
												FGameplayAbilityActivationInfo const ActivationInfo,
												FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	ScheduleFireTrigger(ActivationInfo, AnimInstance);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriangleReloadAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
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
	ensureMsgf(!BuffEffectDataAssets.IsEmpty(), TEXT("GeoTriangleReloadAbility: BuffEffectDataAssets is empty!"));
	if (BuffPickupClass && !BuffEffectDataAssets.IsEmpty())
	{
		int32 const RandomIndex = FMath::RandRange(0, BuffEffectDataAssets.Num() - 1);
		UEffectDataAsset* const SelectedAsset = BuffEffectDataAssets[RandomIndex].LoadSynchronous();
		TArray<TInstancedStruct<FEffectData>> const EffectData =
			UGeoAbilitySystemLibrary::GetEffectDataArray(SelectedAsset);

		AActor* Avatar = GetAvatarActorFromActorInfo();
		FTransform const PickupTransform{Avatar->GetActorLocation()};

		AGeoBuffPickup* Pickup = GetWorld()->SpawnActor<AGeoBuffPickup>(BuffPickupClass, PickupTransform);
		ensureMsgf(IsValid(Pickup), TEXT("GeoTriangleReloadAbility: Failed to spawn AGeoBuffPickup!"));
		if (IsValid(Pickup))
		{
			Pickup->Setup(EffectData, PowerScale);
		}
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}
