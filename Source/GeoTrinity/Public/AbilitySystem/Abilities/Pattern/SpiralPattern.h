// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "SpiralPattern.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API USpiralPattern : public UTickablePattern
{
	GENERATED_BODY()
protected:
	virtual void OnCreate(FGameplayTag AbilityTag) override;
	virtual void InitPattern(FAbilityPayload const& Payload) override;

	virtual void TickPattern(float ServerTime, float SpentTime) override;
	virtual void EndPattern() override;

	UFUNCTION()
	void EndProjectile(AGeoProjectile* Projectile);

	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	float NumberProjectileByRound;
	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	float TimeForOneRound;
	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	float RoundNumber;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(Transient)
	TArray<AGeoProjectile*> Projectiles;

	float ProjectileSpeed;
	float TimeDiffBetweenProjectiles;
	float AngleBetweenProjectiles;
	FVector FirstProjectileOrientation;
};
