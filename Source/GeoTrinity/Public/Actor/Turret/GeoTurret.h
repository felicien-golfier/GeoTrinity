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
	AGeoTurret();

	virtual void InitInteractableData(FInteractableActorData* Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &Data; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	AActor* FindBestTarget() const;
	void ScheduleFire();
	void TryFire();

	UPROPERTY(Replicated)
	FDeployableData Data;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> TurretProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FireInterval = 1.f;

private:
	FTimerHandle FireTimerHandle;
};
