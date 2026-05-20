// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoPillar.generated.h"


/**
 * Pillar deployable spawned by GeoFatalZone when its countdown expires.
 * Has health, can be damaged by players and the boss, and blocks the DevastatingWave.
 * No drain — stays alive until destroyed or explicitly recalled.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoPillar : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoPillar();

	/** Copies Data into the replicated PillarData field, then delegates to Super (which triggers PushAway). */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers PillarData (COND_InitialOnly) in addition to the base-class replicants. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &PillarData; }

private:
	UPROPERTY(Replicated)
	FDeployableData PillarData;
};
