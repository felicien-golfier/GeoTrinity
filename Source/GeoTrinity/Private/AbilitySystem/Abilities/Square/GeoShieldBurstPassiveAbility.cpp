// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoShieldBurstPassiveAbility.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoShieldBurstProjectile.h"
#include "Tool/UGeoGameplayLibrary.h"

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

	if (GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnDamageDealt.AddDynamic(this, &UGeoShieldBurstPassiveAbility::OnDamageDealtCallback);
	}
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

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShieldBurstPassiveAbility::OnDamageDealtCallback(float DamageAmount, FGameplayTag AbilityTag)
{
	GaugeAccumulated += DamageAmount;
	OnGaugeChanged.Broadcast(FMath::Clamp(GaugeAccumulated / GaugeFillThreshold, 0.f, 1.f));

	if (GaugeAccumulated >= GaugeFillThreshold)
	{
		GaugeAccumulated = 0.f;
		OnGaugeChanged.Broadcast(0.f);
		SpawnShieldBurst();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoShieldBurstPassiveAbility::SpawnShieldBurst()
{
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

	StoredPayload = CreateAbilityPayload(StoredPayload.Owner, StoredPayload.Instigator, StoredPayload.Instigator->GetTransform());
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
