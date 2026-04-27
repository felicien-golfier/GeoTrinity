// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoShieldBurstPassiveAbility.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoShieldBurstProjectile.h"
#include "Characters/Component/ShieldBurstPassiveActor.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoShieldBurstPassiveAbility::UGeoShieldBurstPassiveAbility()
{
	SetAssetTags(FGeoGameplayTags::Get().Ability_Spell_ShieldBurst.GetSingleTagContainer());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShieldBurstPassiveAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
													FGameplayAbilityActorInfo const* ActorInfo,
													FGameplayAbilityActivationInfo ActivationInfo,
													FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoShieldBurstPassiveAbility: invalid ASC on activation")))
	{
		return;
	}

	if (!ensureMsgf(
			PassiveActorClass,
			TEXT(
				"UGeoShieldBurstPassiveAbility: PassiveComponentClass is not set — assign BP_ShieldBurstPassiveComponent in the ability defaults")))
	{
		return;
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		AActor* PayloadInstigator = StoredPayload.Instigator;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = PayloadInstigator;
		SpawnParams.Instigator = Cast<APawn>(PayloadInstigator);
		PassiveActor = GetWorld()->SpawnActor<AShieldBurstPassiveActor>(PassiveActorClass, PayloadInstigator->GetTransform(), SpawnParams);
		if (!ensureMsgf(PassiveActor, TEXT("UGeoShieldBurstPassiveAbility: failed to spawn AShieldBurstPassiveActor")))
		{
			return;
		}

		SourceASC->OnDamageDealt.AddDynamic(this, &UGeoShieldBurstPassiveAbility::OnDamageDealtCallback);
	}

	ChargeTimerHandle.Invalidate();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShieldBurstPassiveAbility::EndAbility(FGameplayAbilitySpecHandle Handle,
											   FGameplayAbilityActorInfo const* ActorInfo,
											   FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
											   bool bWasCancelled)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (SourceASC && GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnDamageDealt.RemoveDynamic(this, &UGeoShieldBurstPassiveAbility::OnDamageDealtCallback);
	}

	GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);

	if (PassiveActor)
	{
		PassiveActor->Destroy();
		PassiveActor = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShieldBurstPassiveAbility::OnDamageDealtCallback(float DamageAmount, FGameplayTag AbilityTag)
{
	if (ChargeTimerHandle.IsValid())
	{
		return;
	}

	GaugeAccumulated += DamageAmount;

	float const GaugeRatio = FMath::Clamp(GaugeAccumulated / GaugeFillThreshold, 0.f, 1.f);
	PassiveActor->SetGaugeRatio(GaugeRatio);

	if (GaugeAccumulated >= GaugeFillThreshold)
	{
		GaugeAccumulated = 0.f;
		GetWorld()->GetTimerManager().SetTimer(ChargeTimerHandle, this,
											   &UGeoShieldBurstPassiveAbility::SpawnShieldBurst, ChargeTime, false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShieldBurstPassiveAbility::SpawnShieldBurst()
{
	PassiveActor->SetGaugeRatio(0.f);
	ChargeTimerHandle.Invalidate();

	if (!ensureMsgf(ShieldBurstClass, TEXT("UGeoShieldBurstPassiveAbility: ShieldBurstClass is not set")))
	{
		return;
	}

	AActor* PayloadOwner = StoredPayload.Owner;
	if (!ensureMsgf(PayloadOwner,
					TEXT("UGeoShieldBurstPassiveAbility: StoredPayload.Owner is null in SpawnShieldBurst")))
	{
		return;
	}

	StoredPayload =
		CreateAbilityPayload(StoredPayload.Owner, StoredPayload.Instigator, StoredPayload.Instigator->GetTransform());
	FVector const Origin = FVector(StoredPayload.Origin, ArbitraryCharacterZ);
	TArray<FVector> const Directions =
		GeoASLib::GetTargetDirections(GetWorld(), EProjectileTarget::Forward, StoredPayload.Yaw, Origin);

	if (Directions.Num() != 1)
	{
		ensureMsgf(false,
				   TEXT("UGeoShieldBurstPassiveAbility: We should only have one single direction for that spell (%d)"),
				   Directions.Num());
		return;
	}

	FTransform const SpawnTransform{Directions[0].Rotation().Quaternion(), Origin};

	AGeoShieldBurstProjectile* Projectile = Cast<AGeoShieldBurstProjectile>(
		GeoASLib::StartSpawnProjectile(GetWorld(), ShieldBurstClass, SpawnTransform, StoredPayload, {}));
	if (!ensureMsgf(Projectile, TEXT("UGeoShieldBurstPassiveAbility: failed to spawn GeoShieldBurstProjectile")))
	{
		return;
	}

	Projectile->ShieldAmount = ShieldAmount;
	GeoASLib::FinishSpawnProjectile(GetWorld(), Projectile, SpawnTransform, StoredPayload.ServerSpawnTime,
									FPredictionKey{});
}
