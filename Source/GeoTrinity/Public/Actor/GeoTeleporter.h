// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoTeleporter.generated.h"

class UTextRenderComponent;

/**
 * Overlap pad that teleports the touching playable character to the next AGeoTeleporter sharing its
 * TeleportTag (sorted by actor name, wrapping). CameraTag drives the camera: place AGeoTargetPoints
 * carrying it as camera bounds, and whenever the local player touches a pad (walk-in or arrival) the
 * camera recomputes its bounds from those points.
 */
UCLASS()
class GEOTRINITY_API AGeoTeleporter : public AActor
{
	GENERATED_BODY()

public:
	AGeoTeleporter();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(FTransform const& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<UTextRenderComponent> TextComponent;

	/** Links teleporters (you teleport to the next pad sharing this tag). Editor-created tags work — no native tag
	 * needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FGameplayTag TeleportTag;

	/** Camera bounds of the zone this pad sits in (AGeoTargetPoints carrying this tag). Applied to the local camera
	 * whenever the local player touches the pad (walk-in or arrival). Unset = pad never changes the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FGameplayTag CameraTag;

	/** Label shown above the pad (e.g. to tell teleporters in the same tag group apart in-editor/in-game). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FText DisplayText;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);

	AGeoTeleporter* FindNextTeleporter() const;

	/** Characters another teleporter just sent here — kept until they walk off the pad (end overlap) so no arrival
	 * overlap, however many times it fires, can chain-teleport them back. */
	TSet<TObjectPtr<AActor>> PendingArrivals;
};
