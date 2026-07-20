// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoTeleporter.generated.h"

class UTextRenderComponent;

/**
 * Overlap pad that teleports the touching playable character to the next AGeoTeleporter sharing its
 * TeleportTag (sorted by actor name, wrapping). ArenaTag names the arena on the far side of the trip: the pad makes it
 * the GameState's active arena, which is what reframes the camera and moves the checkpoint.
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

	/** Arena this pad sends the player to; becomes AGeoGameState::ActiveArena on use, which reframes the camera on that
	 * arena's camera-bounds points. Unset = pad never changes arena. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	FGameplayTag ArenaTag;

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
