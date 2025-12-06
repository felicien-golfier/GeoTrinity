// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/GeoAscTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"

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
	virtual void LifeSpanExpired() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FGameplayEffectSpecHandle DamageEffectSpecHandle;
	FDamageEffectParams DamageEffectParams;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

protected:
	/** Functions **/
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	void ApplyEffectToTarget(AActor* OtherActor);
	virtual bool IsValidOverlap(const AActor* OtherActor);
	void StopLoopingSound() const;
	void DisableSphereCollision() const;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
	UFUNCTION()
	void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintCallable)
	virtual void PlayImpactFx() const;

	virtual void EndProjectileLife();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(EditAnywhere, meta = (Tooltip = "Safe guard in case distance check fails"))
	float LifeSpanInSec = 30.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float DistanceSpan = 100.f;

	UPROPERTY(EditAnywhere, Meta = (Bitmask, BitmaskEnum = "ETeam"))
	int32 ApplyEffectToTeamOnOverlap;

	bool bHasOverlapped{false};

	FVector InitialPosition;
	float DistanceSpanSqr;

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