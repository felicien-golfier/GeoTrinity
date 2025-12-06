// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/GeoAscTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "System/GeoPoolableInterface.h"

#include "GeoProjectile.generated.h"

class UProjectileMovementComponent;
class UNiagaraSystem;
class USphereComponent;
class USceneComponent;
class USoundBase;
class UAudioComponent;
class UPrimitiveComponent;
struct FHitResult;

UCLASS()
class GEOTRINITY_API AGeoProjectile
	: public AActor
	, public IGeoPoolableInterface
{
	GENERATED_BODY()
public:
	AGeoProjectile();
	virtual void LifeSpanExpired() override;
	virtual void Tick(float DeltaSeconds) override;

	// IGeoPoolableInterface start
	virtual void Init() override;
	virtual void End() override;
	// IGeoPoolableInterface end

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FGameplayEffectSpecHandle DamageEffectSpecHandle;
	FDamageEffectParams DamageEffectParams;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

protected:
	void ApplyEffectToTarget(AActor* OtherActor);
	virtual bool IsValidOverlap(const AActor* OtherActor);
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
	void InitProjectileMovementComponent();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(EditAnywhere, meta = (Tooltip = "Safe guard in case distance check fails"))
	float LifeSpanInSec = 30.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float DistanceSpan = 100.f;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "ETeam"))
	int32 ApplyEffectToTeamOnOverlap;

	bool bIsEnding{false};

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