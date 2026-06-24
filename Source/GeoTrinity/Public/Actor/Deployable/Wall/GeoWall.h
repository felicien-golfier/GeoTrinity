// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoWall.generated.h"

class UStaticMeshComponent;

/**
 * Deployable wall placed by the Square player.
 * Has a draining health pool (like a turret) and can be damaged, but does not block or push characters
 * and cannot be triggered by stepping on it. It never explodes — UGeoDetonateWallsAbility consumes walls
 * on its ray to boost the ray's damage/shield, then recalls them.
 *
 * Gameplay collision lives on MeshComponent (GeoShape profile), not the root capsule: projectiles and AoE
 * queries hit the wall's mesh shape. The root capsule's collision is disabled in the constructor.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoWall : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoWall(FObjectInitializer const& ObjectInitializer);

	/** Casts Data into WallData and delegates to Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override;
	/** Registers WallData for replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &WallData; }

	// The wall's gameplay collider. Carries the GeoShape profile so projectiles and AoE queries hit the mesh shape
	// instead of the root capsule (whose collision is disabled). Mesh asset is assigned on BP_Wall.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

private:
	UPROPERTY(Replicated)
	FDeployableData WallData;
};
