// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoMine.generated.h"

class AGeoProjectile;

/**
 * Boss mine: sits on one arena tile, arms for as long as its life drain takes to run out, then bursts projectiles in
 * every direction. Shooting it only brings the burst forward — the counterplay is destroying the tile under it, which
 * makes the arena recall the mine with no ground left to stand on, and a mine without ground never detonates.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoMine : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Sets bUseRegularDrain=true and bAutoRecallAtEndLife=true, so the drain doubles as the fuse. */
	AGeoMine(FObjectInitializer const& ObjectInitializer);

	/** Copies Data into the replicated MineData field, then delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers MineData (COND_InitialOnly) in addition to the base-class replicants. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &MineData; }
	/** Server. Bursts BurstProjectileCount projectiles radially, unless the tile under the mine is already gone. */
	virtual void RecallEffect(float Value) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	TSubclassOf<AGeoProjectile> BurstProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "1"))
	int32 BurstProjectileCount = 12;

private:
	UPROPERTY(Replicated)
	FDeployableData MineData;
};
