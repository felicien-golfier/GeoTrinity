// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "GeoGameMode.generated.h"

class AEnemyCharacter;
class UStatusInfo;
/**
 * Game mode for GeoTrinity. Assigns starting player classes based on join order
 * and handles server-side round management.
 */
UCLASS()
class GEOTRINITY_API AGeoGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AGeoGameMode(FObjectInitializer const& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
};
