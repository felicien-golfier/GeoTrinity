// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoFloorButton.generated.h"

class UBoxComponent;
class UTextRenderComponent;

/**
 * Overlap pad a playable character presses by walking onto it, labelled by text lying flat under their feet. The
 * overlap fires on every machine but only the server runs Press, so pressing a pad is always a request the authority
 * grants — what granting it means is the subclass's to say.
 */
UCLASS(Abstract)
class GEOTRINITY_API AGeoFloorButton : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the TriggerBox collision volume and the Label text renderer rotated to face the top-down camera. */
	AGeoFloorButton();

protected:
	/** Binds the OnBeginOverlap delegate to the trigger volume. */
	virtual void BeginPlay() override;

	/** Server. Runs when a playable character steps on this pad. */
	virtual void Press() PURE_VIRTUAL(AGeoFloorButton::Press, );

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCollision")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Label oriented to face up toward the top-down camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoDisplay")
	TObjectPtr<UTextRenderComponent> Label;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
};
