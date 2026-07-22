// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoBombZone.generated.h"

/**
 * Boss bomb dropped on a player by UGeoTileBombAbility. Two phases run off the base drain/blink lifecycle:
 *   - Drain (LifeDrainMaxDuration): the bomb stays attached to the carrier and rides them around.
 *   - Blink (BlinkDuration): the bomb detaches, planting where the carrier stood, and flashes as its telegraph.
 * When the blink ends it recalls: the effect data hits everyone within Size, then the tiles under it are carved. Not
 * damageable — the counterplay is to walk the carrier's plant to the rim and then clear the blast before it goes off.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoBombZone : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Sets the drain fuse (bUseRegularDrain, bAutoRecallAtEndLife, bExplodeAtRecall) and makes the bomb un-damageable.
	 */
	AGeoBombZone(FObjectInitializer const& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	/** Copies Data into the replicated BombData field, then delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers BombData (COND_InitialOnly) in addition to the base-class replicants. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FGameplayCueParameters GetSpawnCueParams(FGameplayTag SoundTag = FGameplayTag()) override;


protected:
	virtual FDeployableData const* GetData() const override { return &BombData; }
	/** Server. Explodes (base effect data within Size), then carves the arena tiles under the plant. */
	virtual void ExplodeEffect(float const Value) override;

	virtual void StartBlinking() override;

private:
	UPROPERTY(Replicated)
	FDeployableData BombData;
};
