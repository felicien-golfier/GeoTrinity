// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoShieldBurstPassiveAbility.generated.h"

class AGeoShieldBurstProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGaugeChanged, float, GaugeRatio);

/**
 * Passive ability for the Square player.
 * Auto-attack damage fills a gauge; at 100% a shield burst projectile is launched toward allies.
 * Broadcasts OnGaugeChanged so the character widget can display the fill in real time.
 */
UCLASS()
class GEOTRINITY_API UGeoShieldBurstPassiveAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnGaugeChanged OnGaugeChanged;

private:
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

	float GaugeAccumulated = 0.f;
};
