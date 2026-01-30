// Fill out your copyright notice in the Description page of Project Settings.

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

	UFUNCTION(Server, Reliable)
	void ServerSetAimYaw(float YawDegrees);

protected:
	virtual void BeginPlay() override;

public:
	static AGeoPlayerController* GetLocalGeoPlayerController(UWorld const* World);

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
};
