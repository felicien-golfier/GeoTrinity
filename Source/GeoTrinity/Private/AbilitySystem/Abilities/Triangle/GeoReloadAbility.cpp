// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Deployable/GeoBuffPickup.h"
#include "GenericTeamAgentInterface.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoReloadAbility::OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		ensureMsgf(ASC, TEXT("GeoReloadAbility::OnGiveAbility: No ASC"));
		return;
	}

	FGameplayAbilitySpecHandle const SpecHandle = Spec.Handle;
	ASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetAmmoAttribute())
		.AddWeakLambda(this,
					   [this, ASC, SpecHandle](FOnAttributeChangeData const& Data)
					   {
						   if (Data.NewValue > 0.f)
						   {
							   return;
						   }
						   CastChecked<UGeoAbilitySystemComponent>(ASC)->TryActivateAbilityWithTargetData(SpecHandle);
					   });
}

bool UGeoReloadAbility::CheckCost(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
								  FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	float const CurrentAmmo = ASC->GetNumericAttribute(UCharacterAttributeSet::GetAmmoAttribute());
	float const MaxAmmo = ASC->GetNumericAttribute(UCharacterAttributeSet::GetMaxAmmoAttribute());
	if (CurrentAmmo == MaxAmmo)
	{
		return false;
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
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
		// Index is set with the number of ammo remaining, except when out of ammo.
		int32 const Index =
			(CurrentAmmo == 0.f ? AbilityTargetData.Seed : FMath::RoundToInt(CurrentAmmo)) % BuffEffects.Num();

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
			PickupData.Level = FMath::RoundToInt32(PowerScale * 10.f);
			PickupData.EffectDataArray = {BuffEffects[Index]};
			PickupData.PowerScale = PowerScale;
			PickupData.MeshIndex = Index;
			PickupData.TargetLocation = AvatarLocation + SpawnOffset;
			PickupData.Seed = StoredPayload.Seed;
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
