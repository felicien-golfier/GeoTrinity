// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoMine.generated.h"

/**
 * Deployable mine placed by the Square player.
 * Triggers an explosion when any actor steps on it:
 * — enemies take damage scaled by Params.Value (LifeSpent)
 * — allies gain shield scaled by Params.Value (LifeSpent)
 * Explosion radius is driven by Params.Size.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoMine : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoMine();

	/** Casts Data to FDeployableData, stores it in MineData, resets the recall flag, then calls Super. */
	virtual void InitInteractableData(FInteractableActorData* Data) override;
	/** Registers MineData with initial-only replication (replicated once at spawn). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Detonates the mine: applies damage to enemies and shield to allies within Params.Size radius.
	 * Both effects are scaled by Value * Params.Value. No-ops when already recalled or expired.
	 *
	 * @param Value  Scale factor for the detonation magnitude (1.0 = full detonation).
	 */
	virtual void Recall(float Value) override;

protected:
	virtual FDeployableData const* GetData() const override { return &MineData; }

	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							   UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep,
							   FHitResult const& SweepResult);

	UPROPERTY(Replicated)
	FDeployableData MineData;
	
	bool bIsRecalling = false;
	
};
