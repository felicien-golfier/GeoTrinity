// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StructUtils/InstancedStruct.h"
#include "System/GeoPoolableInterface.h"

#include "GeoProjectile.generated.h"

class UGeoAbilitySystemComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class USphereComponent;
class USceneComponent;
class USoundBase;
class UAudioComponent;
class UPrimitiveComponent;
struct FHitResult;
enum class ETeam : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileEndLife, AGeoProjectile*, Projectile);
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

	TArray<TInstancedStruct<struct FEffectData>> EffectDataArray;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

	FAbilityPayload Payload;

	FOnProjectileEndLife OnProjectileEndLifeDelegate;

protected:
	virtual bool IsValidOverlap(AActor const* OtherActor);

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep,
						 FHitResult const& SweepResult);
	UFUNCTION()
	void OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					 FVector NormalImpulse, FHitResult const& Hit);

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

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeam"))
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
