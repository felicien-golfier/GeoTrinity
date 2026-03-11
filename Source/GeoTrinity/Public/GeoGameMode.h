// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "GeoGameMode.generated.h"

class AEnemyCharacter;
class UStatusInfo;
/**
 *
 */
UCLASS()
class GEOTRINITY_API AGeoGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AGeoGameMode(FObjectInitializer const& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
};
