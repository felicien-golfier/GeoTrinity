// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoTurret.generated.h"

class AGeoProjectile;
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
	virtual FDeployableData const* GetData() const override { return &Data; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Returns the owner's last basic-ability target when it is still a valid in-range hostile, otherwise the nearest
	 * hostile within range, or nullptr if none is found.
	 */
	AActor* FindBestTarget() const;

	/** Fires a projectile at the current best target if one exists. */
	void TryFire();
	virtual void Expire(float TimeBeforeDestroy) override;

	UPROPERTY(Replicated)
	FDeployableData Data;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> TurretProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FireInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	bool bOverrideDistanceSpan = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (ClampMin = "0", AllowPrivateAccess = true, EditCondition = "bOverrideDistanceSpan",
					  EditConditionHides = "true", UIMin = "0"))
	float DistanceSpan = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	bool bOverrideSpeed = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (ClampMin = "0", AllowPrivateAccess = true, EditCondition = "bOverrideSpeed",
					  EditConditionHides = "true", UIMin = "0"))
	float ProjectileSpeed = 4000.f;


private:
	// Set on the server each tick from FindBestTarget; replicated so clients can orient toward the live target
	// location.
	UPROPERTY(Replicated)
	TObjectPtr<AActor> CurrentTarget;

	FTimerHandle FireTimerHandle;
};
