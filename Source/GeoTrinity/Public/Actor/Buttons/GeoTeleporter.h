// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoTeleporter.generated.h"

class APlayableCharacter;
class UTextRenderComponent;

/**
 * Overlap pad that teleports the touching playable character — and every other player sharing its machine, since couch
 * coop players share one camera — to the next AGeoTeleporter sharing its TeleportTag (sorted by actor name, wrapping).
 * Purely a position move — the camera reframes on its own because the arriving player overlaps the destination room's
 * AGeoCameraVolume.
 * Inert while the match is in progress: no leaving (or joining) a live fight through a pad.
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCollision")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCollision")
	TObjectPtr<UTextRenderComponent> TextComponent;

	/** Links teleporters (you teleport to the next pad sharing this tag). Editor-created tags work — no native tag
	 * needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport")
	FGameplayTag TeleportTag;

	/** Label shown above the pad (e.g. to tell teleporters in the same tag group apart in-editor/in-game). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport")
	FText DisplayText;

	/** Seconds a traveller's move input stays ignored once it lands on this pad, so the camera reframes before they can
	 * walk off — and back onto a pad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport", meta = (ClampMin = "0.01"))
	float ArrivalMoveLockDuration = 0.5f;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);

	AGeoTeleporter* FindNextTeleporter() const;

	/** Stops Traveller and ignores its move input for ArrivalMoveLockDuration. Runs on its arrival overlap, after the
	 * teleport, on every machine holding its controller. */
	void LockArrivalMovement(APlayableCharacter& Traveller) const;

	/** Characters another teleporter just sent here — kept until they walk off the pad (end overlap) so no arrival
	 * overlap, however many times it fires, can chain-teleport them back. */
	TSet<TObjectPtr<AActor>> PendingArrivals;
};
