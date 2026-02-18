// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "GeoProjectileAbility.generated.h"

enum class EProjectileTarget : uint8;
class AGeoProjectile;
/**
 * A spell that launches a projectile
 */
UCLASS()
class GEOTRINITY_API UGeoProjectileAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void Fire() override;


	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectileUsingDirection(FVector const& Direction);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(FTransform const SpawnTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectilesUsingTarget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	FAbilityPayload StoredPayload;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Target")
	EProjectileTarget Target;
};
