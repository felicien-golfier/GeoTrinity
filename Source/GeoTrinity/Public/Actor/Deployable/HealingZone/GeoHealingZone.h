// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoHealingZone.generated.h"


/**
 * Deployable healing zone placed by the Circle player.
 * Periodically heals allies who remain inside the capsule area. Can be absorbed by UGeoMoiraBeamAbility
 * which drains its health and converts it into fuel, radius growth, and damage/heal boost.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoHealingZone : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Initializes drain behavior: bUseRegularDrain=false and marks the zone non-damageable so only GAS effects alter its health. */
	AGeoHealingZone(FObjectInitializer const& ObjectInitializer);
	/** Sets capsule size from Data.Params.Size and binds overlap delegates to track actors inside the zone. */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers Data (COND_InitialOnly) for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Returns health / max-health ratio (0–1); drives the health bar. Zero if max-health is not yet initialized. */
	virtual float GetDurationPercent() const override;

protected:
	virtual FDeployableData const* GetData() const override { return &Data; }

	virtual void InitDrain() override;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnRep_Data() const;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FDeployableData Data;

	TSet<TWeakObjectPtr<AActor>> ActorsInZone;
};
