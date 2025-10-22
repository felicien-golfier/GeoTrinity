// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "GeoGameMode.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API AGeoGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AGeoGameMode(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
};
