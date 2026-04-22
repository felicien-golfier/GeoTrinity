// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoShieldBurstPassiveAbility.generated.h"

class AGeoShieldBurstProjectile;
class UShieldBurstPassiveComponent;

/**
 * Passive ability for the Square player.
 * Auto-attack damage fills a gauge; at 100% a shield burst projectile is launched toward allies after a charge delay.
 * GaugeRatio is replicated via UShieldBurstPassiveComponent so all clients can drive the charge visual.
 */
UCLASS()
class GEOTRINITY_API UGeoShieldBurstPassiveAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

	UGeoShieldBurstPassiveAbility();

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	UFUNCTION()
	void OnDamageDealtCallback(float DamageAmount, FGameplayTag AbilityTag);

	void SpawnShieldBurst();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|ShieldBurst", meta = (AllowPrivateAccess = true))
	float GaugeFillThreshold = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|ShieldBurst", meta = (AllowPrivateAccess = true))
	float ShieldAmount = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|ShieldBurst", meta = (AllowPrivateAccess = true))
	TSubclassOf<AGeoShieldBurstProjectile> ShieldBurstClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|ShieldBurst", meta = (AllowPrivateAccess = true))
	TSubclassOf<UShieldBurstPassiveComponent> PassiveComponentClass;

	float GaugeAccumulated = 0.f;
	FTimerHandle ChargeTimerHandle;

	UPROPERTY()
	TObjectPtr<UShieldBurstPassiveComponent> PassiveComponent;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|ShieldBurst")
	float ChargeTime = 1.f; // Needs to be accessible from the passive comp via CDO
};
