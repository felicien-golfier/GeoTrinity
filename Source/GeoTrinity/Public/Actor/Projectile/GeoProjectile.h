// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileEndLife, AGeoProjectile*, Projectile);
UCLASS()
class GEOTRINITY_API AGeoProjectile : public AActor
{
	GENERATED_BODY()
public:
	AGeoProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(AActor const* RealViewer, AActor const* ViewTarget,
								  FVector const& SrcLocation) const override;
	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void InitProjectileLife();

	void AdvanceProjectile(float TimeDelta);
	UFUNCTION(BlueprintCallable)
	void SetDistanceSpan(float Distance);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadOnly)
	TArray<TInstancedStruct<struct FEffectData>> EffectDataArray;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

	UPROPERTY(BlueprintReadOnly)
	FAbilityPayload Payload;

	UPROPERTY(Replicated)
	int16 PredictionKeyId = 0;

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

	/**
	 * Called on every machine that has this projectile when it hits a valid actor.
	 * Override in Blueprint to apply a hit flash or other cosmetic reaction on the HitActor's mesh.
	 *
	 * @param HitActor  The actor that was struck. Cast to GeoCharacter to access its mesh.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|GameFeel")
	void OnProjectileHit(AActor* HitActor);

	virtual void EndProjectileLife();
	void InitProjectileMovementComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (Tooltip = "Safe guard in case distance check fails", AllowPrivateAccess = true))
	float LifeSpanInSec = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (ClampMin = "0", AllowPrivateAccess = true))
	float DistanceSpan = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag", AllowPrivateAccess = true))
	int32 OverlapAttitude = static_cast<int32>(ETeamAttitudeBitflag::Hostile);

	bool bIsEnding{false};

	FVector InitialPosition;
	float DistanceSpanSqr;

	/** Cosmetic (let the juice flow) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoProjectile", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundBase> LoopingSound;
};
