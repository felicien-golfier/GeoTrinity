// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"

#include "GeoPlayerController.generated.h"

/**
 *
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
	static AGeoPlayerController* GetLocalGeoPlayerController(UWorld const* World);

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

};
