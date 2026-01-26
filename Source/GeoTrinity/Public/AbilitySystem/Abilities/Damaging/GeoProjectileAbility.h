// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "GeoProjectileAbility.generated.h"

UENUM(BlueprintType)
enum class ETarget : uint8
{
	Forward,
	AllPlayers
};

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
		FGameplayAbilityActivationInfo ActivationInfo, FGameplayEventData const* TriggerEventData) override;
	void OnMontageSectionStartEnded();

	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
		FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectileUsingLocation(FVector const& projectileTargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(FTransform const& SpawnTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectilesUsingTarget();

	UFUNCTION(BlueprintCallable, Category = "Ability|Target")
	TArray<FVector> GetTargetLocations() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	FAbilityPayload StoredPayload;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Target")
	ETarget Target = ETarget::Forward;

	FTimerHandle StartSectionTimerHandle;
};
