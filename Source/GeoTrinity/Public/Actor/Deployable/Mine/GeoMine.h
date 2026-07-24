// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoMine.generated.h"

class AGeoProjectile;

/**
 * Boss mine: sits on one arena tile and runs a fuse. If it survives the fuse it bursts projectiles in every direction.
 * Its health is its life, not a fuse — players defuse it by destroying it (health to zero) before the fuse ends, and
 * dropping the tile under it recalls it into the void. Only running out the fuse detonates it.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoMine : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Disables the regular drain (health is the mine's life, not the fuse) and sets bAutoRecallAtEndLife=true so the
	 * fuse detonates. */
	AGeoMine(FObjectInitializer const& ObjectInitializer);

	/** Copies Data into the replicated MineData field, then delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers MineData (COND_InitialOnly) in addition to the base-class replicants. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &MineData; }
	/** Server. Arms the fuse timer (LifeDrainMaxDuration) instead of the base life drain. */
	virtual void InitDrain() override;
	/** Health reaching zero means players destroyed the mine: it defuses (Expire), never bursts. */
	virtual void OnHealthChanged_Implementation(float NewValue) override;
	/** Server. Reached only when the fuse runs out: bursts BurstProjectileCount projectiles radially. */
	virtual void RecallEffect(float Value) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	TSubclassOf<AGeoProjectile> BurstProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine", meta = (ClampMin = "1"))
	int32 BurstProjectileCount = 12;

private:
	/** Fires when the fuse elapses without the mine being destroyed: starts the blink, then detonates. */
	void OnFuseElapsed();

	FTimerHandle FuseTimerHandle;

	UPROPERTY(Replicated)
	FDeployableData MineData;
};
