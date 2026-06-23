// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Actor/Deployable/BuffPickup/GeoBuffPickup.h"
#include "GenericTeamAgentInterface.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

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
						   CastChecked<UGeoAbilitySystemComponent>(ASC)->TryActivateAbilityWithTargetData(
							   SpecHandle, GetAbilityTag());
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
	if (!GeoLib::IsServer(GetWorld()))
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

		FRandomStream Rng(StoredPayload.Seed);
		float const RandomAngle = Rng.FRandRange(0.f, 2.f * PI);
		float const RandomRadius = Rng.FRandRange(MinSpawnRadius, MaxSpawnRadius);
		FVector const InstigatorLocation = StoredPayload.Instigator->GetActorLocation();
		FVector const SpawnOffset = GetReachableSpawnOffset(
			InstigatorLocation, {FMath::Cos(RandomAngle) * RandomRadius, FMath::Sin(RandomAngle) * RandomRadius, 0.f});
		FTransform const SpawnTransform{InstigatorLocation};

		AGeoBuffPickup* Pickup = GetWorld()->SpawnActorDeferred<AGeoBuffPickup>(
			BuffPickupClass, SpawnTransform, StoredPayload.Instigator, Cast<APawn>(StoredPayload.Instigator),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!ensureMsgf(IsValid(Pickup), TEXT("GeoTriangleReloadAbility: Failed to spawn AGeoBuffPickup!")))
		{
			EndAbility(true, true);
			return;
		}

		FBuffPickupData PickupData;
		GeoASLib::FillDeployableData(PickupData, StoredPayload, BuffEffects, FDeployableDataParams());
		PickupData.Level = FMath::RoundToInt32(PowerScale * 10.f);
		PickupData.EffectDataArray = {BuffEffects[Index]};
		PickupData.PowerScale = PowerScale;
		PickupData.BuffIndex = Index;
		PickupData.TargetLocation = InstigatorLocation + SpawnOffset;

		Pickup->InitInteractable(&PickupData);
		Pickup->FinishSpawning(SpawnTransform);
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
}

// ---------------------------------------------------------------------------------------------------------------------
FVector UGeoReloadAbility::GetReachableSpawnOffset(FVector Origin, FVector DesiredOffset) const
{
	FVector const TargetLocation = Origin + DesiredOffset;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(StoredPayload.Instigator);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Origin, TargetLocation, ECC_GeoCharacter, QueryParams))
	{
		return DesiredOffset;
	}

	float const ReachableDistance = FMath::Max(0.f, Hit.Distance - PickupRadius);
	return DesiredOffset.GetSafeNormal() * ReachableDistance;
}

// ---------------------------------------------------------------------------------------------------------------------
FLinearColor UGeoReloadAbility::GetColorForIndex(int32 Index) const
{
	if (BuffColors.IsEmpty())
	{
		return FLinearColor::White;
	}
	return BuffColors[((Index % BuffColors.Num()) + BuffColors.Num()) % BuffColors.Num()];
}

// ---------------------------------------------------------------------------------------------------------------------
FLinearColor UGeoReloadAbility::GetColorForAmmo(int32 Ammo)
{
	// Resolve the configured (Blueprint-derived) reload ability CDO that owns the authored palette. The ability catalog
	// is keyed by the Spell AbilityTag, which has no native constant here, so find the entry by class instead.
	UAbilityInfo const* AbilityInfo = GeoASLib::GetAbilityInfo();
	if (!AbilityInfo)
	{
		return FLinearColor::White;
	}

	for (FGameplayAbilityInfo const& Info : AbilityInfo->GetAllAbilityInfos())
	{
		if (Info.AbilityClass && Info.AbilityClass->IsChildOf(UGeoReloadAbility::StaticClass()))
		{
			return Info.AbilityClass->GetDefaultObject<UGeoReloadAbility>()->GetColorForIndex(Ammo);
		}
	}
	return FLinearColor::White;
}
