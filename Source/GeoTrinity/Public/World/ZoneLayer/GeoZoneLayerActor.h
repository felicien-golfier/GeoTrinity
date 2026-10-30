// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoZoneLayerActor.generated.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The two planes of the zone layer (World/ZoneLayer/CLAUDE.md), spawned by UGeoZoneLayerSubsystem: MI_ZoneLayer_Ground
 * under MI_ZoneLayer_Telegraph, both at GeoZoneLayer::ZoneLayerZ, wide enough to cover any arena from the origin. The
 * materials lay everything over world XY, so the planes never move.
 */
UCLASS(NotPlaceable, Transient)
class GEOTRINITY_API AGeoZoneLayerActor : public AActor
{
	GENERATED_BODY()

public:
	/** Builds both planes: engine plane mesh, no collision, no shadow, each at its sort priority. */
	AGeoZoneLayerActor();

	/** Gives each plane its material from Game Data Settings, reading SlotTexture and the colour palette. */
	void InitPlanes(UTexture2D* SlotTexture);

	/** How many slots of the slot texture the materials loop over. */
	void SetZoneCount(int32 ZoneCount) const;

private:
	/** One plane, hidden until InitPlanes gives it its material. */
	UStaticMeshComponent* CreatePlane(FName Name, int32 SortPriority, UStaticMesh* Mesh);

	UPROPERTY(VisibleAnywhere, Category = "GeoZoneLayer")
	TObjectPtr<UStaticMeshComponent> GroundPlane;

	UPROPERTY(VisibleAnywhere, Category = "GeoZoneLayer")
	TObjectPtr<UStaticMeshComponent> TelegraphPlane;
};
