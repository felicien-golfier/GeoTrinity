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
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;

	virtual void AnimTrigger() override;


	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectileUsingLocation(const FVector& projectileTargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(const FTransform SpawnTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectilesUsingTarget();

	UFUNCTION(BlueprintCallable, Category = "Ability|Target")
	TArray<FVector> GetTargetLocations() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	FAbilityPayload StoredPayload;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Target")
	ETarget Target = ETarget::Forward;

	/** If true, spawns projectile from a socket named by the current montage section index (e.g., section "Fire2" uses
	 * socket "2") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Projectile")
	bool bUseSocketFromSectionIndex = false;
};
