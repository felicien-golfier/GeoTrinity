// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"

#include "GeoPlayerController.generated.h"

/**
 * Player controller for GeoTrinity. Configures the camera view target and Enhanced Input
 * mapping context on BeginPlay. In non-shipping builds also drives the per-frame combat stats update.
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AGeoPlayerController(FObjectInitializer const& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

#if !UE_BUILD_SHIPPING
	virtual void Tick(float DeltaTime) override;
#endif

public:
	/** Returns the first local AGeoPlayerController found in World, or nullptr if none exists. */
	static AGeoPlayerController* GetLocalGeoPlayerController(UWorld const* World);

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

};
