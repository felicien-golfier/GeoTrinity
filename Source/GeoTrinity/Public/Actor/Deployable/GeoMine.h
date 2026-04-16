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

	/** Casts Data to FDeployableData, stores it in MineData, and calls Super to start the drain. */
	virtual void InitInteractableData(FInteractableActorData* Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Triggers the mine explosion: damages enemies and shields allies within Params.Size radius.
	 * The final damage/shield magnitude is Params.Value * Value — pass 1.f for full-strength detonation.
	 * Also called internally from the step-on overlap with Value = 1.f.
	 *
	 * @param Value  Multiplier applied to the base LifeSpent magnitude. Range: [0, 1].
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
