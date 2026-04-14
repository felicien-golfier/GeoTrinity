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

	/** Casts Data to FDeployableData and stores it in MineData. Must be called before the mine is active. */
	virtual void InitInteractableData(FInteractableActorData* Data) override;
	/** Registers MineData for initial-only replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Sphere-overlaps all actors within Params.Size: deals damage to enemies and grants shield to allies,
	 * both scaled by Params.Value * Value, then calls Super::Recall to destroy the mine.
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
