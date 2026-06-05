// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoMatchStateButton.generated.h"

class UBoxComponent;
class UTextRenderComponent;

/** Match-state transition this button requests from the game mode when triggered. */
UENUM(BlueprintType)
enum class EGeoMatchStateRequest : uint8
{
	StartMatch,
	WaitingToStart,
	WaitingPostMatch
};

/**
 * Overlap button that requests a match-state transition from the game mode when a playable
 * character touches it. Server-only, mirroring AGeoClassChangeTrigger.
 */
UCLASS()
class GEOTRINITY_API AGeoMatchStateButton : public AActor
{
	GENERATED_BODY()

public:
	AGeoMatchStateButton();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<UBoxComponent> TriggerBox;

	/** Label oriented to face up toward the top-down camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match")
	EGeoMatchStateRequest StateRequest = EGeoMatchStateRequest::StartMatch;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
};
