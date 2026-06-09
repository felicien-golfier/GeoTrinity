// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "GeoMainMenuGameMode.generated.h"

class AGeoMainMenuPlayerController;

/**
 * GameMode for the main menu level. Spawns no pawn; delegates all UI logic to AGeoMainMenuPlayerController.
 */
UCLASS()
class GEOTRINITY_API AGeoMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGeoMainMenuGameMode(FObjectInitializer const& ObjectInitializer);
};
