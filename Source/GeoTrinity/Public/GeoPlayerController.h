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
	AGeoPlayerController( const FObjectInitializer& ObjectInitializer );

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess( APawn* APawn ) override;

public:
	UPROPERTY( EditAnywhere, Category = "Input" )
	TSoftObjectPtr< UInputMappingContext > InputMapping;

	UPROPERTY( EditAnywhere, Category = "Input" )
	UInputAction* MoveAction;
};
