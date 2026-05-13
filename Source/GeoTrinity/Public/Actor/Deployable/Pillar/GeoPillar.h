// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoPillar.generated.h"

/** Runtime init data for a pillar deployable. */
USTRUCT()
struct FPillarData : public FDeployableData
{
	GENERATED_BODY()
};

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

	virtual void InitInteractable(FInteractableActorData* Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &PillarData; }

	virtual void OnHealthChanged_Implementation(float NewValue) override;

	/** Fired on health reaching zero — override in BP for death VFX. */
	UFUNCTION(BlueprintNativeEvent)
	void OnPillarDestroyed();
	virtual void OnPillarDestroyed_Implementation();

private:
	UPROPERTY(Replicated)
	FPillarData PillarData;
};
