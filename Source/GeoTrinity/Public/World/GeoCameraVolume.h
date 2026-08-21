// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoCameraVolume.generated.h"

class UBoxComponent;

/**
 * Two boxes, one job: TriggerBox is where the local player has to stand for this volume to frame the camera, and
 * BoundsBox is what the camera may then show — the view is kept inside it and the camera zooms to OrthoWidth. With
 * bLimitView off the volume only sets the zoom and BoundsBox is hidden. Framing is therefore pure location — no
 * arena, no boss, no match state. Leave the hub and corridors uncovered to let the camera follow freely. Purely
 * local — the overlap is a client-side query.
 */
UCLASS()
class GEOTRINITY_API AGeoCameraVolume : public AActor
{
	GENERATED_BODY()

public:
	/** Creates TriggerBox as the root component and BoundsBox under it. Overlaps are bound in BeginPlay. */
	AGeoCameraVolume();

	/** World-space XY footprint of BoundsBox: the rectangle the camera view is kept inside while this volume is
	 * active. Invalid while bLimitView is off — nothing to clamp to. */
	FBox2D GetViewBounds() const;

	/** OrthoWidth the camera interpolates to while inside this volume. */
	float GetOrthoWidth() const { return OrthoWidth; }

protected:
	virtual void BeginPlay() override;

	/** Hides BoundsBox while it has no say over the view. */
	virtual void OnConstruction(FTransform const& Transform) override;

	/** Where the local player has to stand for this volume to take the camera over. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCamera")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** What the camera may show: the view stops on this box's sides. Collides with nothing — a marker, not a trigger.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCamera")
	TObjectPtr<UBoxComponent> BoundsBox;

	/** Whether BoundsBox limits the view. Off = this volume only sets the zoom and the camera keeps following freely,
	 * and BoundsBox is hidden. */
	UPROPERTY(EditAnywhere, Category = "GeoCamera")
	bool bLimitView = true;

	/** Framing width of the room, zoomed to on entering. Coop players spreading out still widen the view past it. */
	UPROPERTY(EditAnywhere, Category = "GeoCamera", meta = (ClampMin = "1.0"))
	float OrthoWidth = 3000.f;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);
};
