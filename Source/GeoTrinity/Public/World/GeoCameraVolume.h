// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoCameraVolume.generated.h"

class UBoxComponent;

/**
 * Box trigger that selects the camera's bounds while the local player stands inside it. It carries one tag, ArenaTag,
 * and hands it to AGeoGameCamera on overlap; the camera frames the `TargetPoint.CameraBounds` corner points that carry
 * that tag — the same bounds computation used before, now driven by where the player is rather than by an active arena.
 * Framing is therefore pure location: no boss, no match state. Place one volume per room you want framed; leave the
 * hub/corridors uncovered to let the camera follow freely. Purely local — the overlap is a client-side query.
 */
UCLASS()
class GEOTRINITY_API AGeoCameraVolume : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the TriggerBox as the root component. Overlap callbacks are bound in BeginPlay. */
	AGeoCameraVolume();

	/** The Arena.* tag whose `TargetPoint.CameraBounds` corner points frame the camera while this volume is active. */
	FGameplayTag GetArenaTag() const { return ArenaTag; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCamera")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Names which `TargetPoint.CameraBounds` corner points to frame — the same Arena.* tag those points already carry.
	 */
	UPROPERTY(EditAnywhere, Category = "GeoCamera")
	FGameplayTag ArenaTag;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);
};
