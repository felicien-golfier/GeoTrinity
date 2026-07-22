// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoTeleporter.generated.h"

class UTextRenderComponent;

/**
 * Overlap pad that teleports the touching playable character to the next AGeoTeleporter sharing its
 * TeleportTag (sorted by actor name, wrapping). Purely a position move — the camera reframes on its own because the
 * arriving player overlaps the destination room's AGeoCameraVolume.
 */
UCLASS()
class GEOTRINITY_API AGeoTeleporter : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the MeshComponent, TextComponent, and overlap collision used to detect arriving characters. */
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
