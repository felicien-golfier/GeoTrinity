// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/GeoInteractableActor.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoTurret.generated.h"

class UCapsuleComponent;

/**
 * Deployable turret that periodically fires projectiles at the nearest hostile actor.
 * Configured via FDeployableData supplied by the deploy ability at spawn time.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoTurret : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Enables continuous Tick at default interval. */
	AGeoTurret(FObjectInitializer const& ObjectInitializer);

	/** Copies Data into the replicated Data field, then delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers Data (COND_InitialOnly) and CurrentTarget for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Returns the turret's replicated data block. */
	virtual FDeployableData const* GetData() const override { return &Data; }

	/** Arms the repeating fire timer: on every machine for an ally turret, on the server alone for an enemy one. */
	virtual void BeginPlay() override;
	/** Recomputes the best target each frame and writes CurrentTarget for client mesh orientation. */
	virtual void Tick(float DeltaSeconds) override;
	/** Clears the fire timer before the actor is torn down. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Returns the owner's last basic-ability target when it is still a valid in-range hostile, otherwise the nearest
	 * hostile within range, or nullptr if none is found.
	 */
	AActor* FindBestTarget() const;

	/** Aims at the current target, if there is one, and fires at it: an ally turret on this machine alone, an enemy
	 * turret on every machine through MulticastFire. */
	void TryFire();

	/** Enemy turret. Fires along the server's aim on every machine, each as it hears of it, since each machine sees
	 * the player it targets somewhere else. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(float Yaw);

	/** Fires one bullet along Yaw on this machine: the server judges it, the others draw it. */
	void Fire(float Yaw);

	/** True for a player's turret. It only ever targets enemies, which stand where the server has them on every
	 * machine, so each machine can aim and fire on its own. */
	bool IsPlayerTurret() const;
	/** Clears the fire timer, then delegates to Super. */
	virtual void Expire(bool bForce) override;

	UPROPERTY(Replicated)
	FDeployableData Data;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable|Projectile",
			  meta = (AllowPrivateAccess = true))
	FExternalProjectileParams ProjectileParams;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDeployable|Projectile",
			  meta = (AllowPrivateAccess = true))
	float FireInterval = 1.f;


private:
	// Set on the server each tick from FindBestTarget; replicated so clients can orient toward the live target
	// location.
	UPROPERTY(Replicated)
	TObjectPtr<AActor> CurrentTarget;

	FTimerHandle FireTimerHandle;
};
