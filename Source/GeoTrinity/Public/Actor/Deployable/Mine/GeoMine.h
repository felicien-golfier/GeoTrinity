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

	/** Casts Data into MineData and delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;

	/**
	 * Applies damage to hostiles and shield to allies, both scaled by Value (life-spent ratio).
	 * Called once per valid target inside Explode() on the server.
	 */
	virtual void ApplyExplodeEffect(float Value, UGeoAbilitySystemComponent* SourceASC, AActor* Actor,
									UGeoAbilitySystemComponent* TargetASC) override;
	/** Registers MineData for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
};
