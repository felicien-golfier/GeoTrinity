// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "GameFramework/Actor.h"
#include "GeoProjectile.generated.h"

class UProjectileMovementComponent;
class UNiagaraSystem;
class USphereComponent;

UCLASS()
class GEOTRINITY_API AGeoProjectile : public AActor
{
	GENERATED_BODY()
public:
	AGeoProjectile();
	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = true))
	FGameplayEffectSpecHandle DamageEffectSpecHandle;
	FDamageEffectParams DamageEffectParams;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;
	
protected:
	/** Functions **/
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	void ApplyEffectToTarget(AActor* OtherActor);
	bool IsValidOverlap(const AActor* OtherActor);
	void StopLoopingSound() const;
	
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherOverlappedComponent,
		int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
	
	UFUNCTION(BlueprintCallable)
	virtual void PlayImpactFx() const;
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;
	
	UPROPERTY(EditAnywhere)
	float LifeSpanInSec = 15.f;
	
	bool bHit {false};

	/** Cosmetic (let the juice flow) **/
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> LoopingSound;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;
	
	UPROPERTY(EditAnywhere)
	FString ProjectileName;
};