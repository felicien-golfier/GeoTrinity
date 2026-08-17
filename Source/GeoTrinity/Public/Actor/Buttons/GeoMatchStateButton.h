// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Buttons/GeoFloorButton.h"
#include "CoreMinimal.h"

#include "GeoMatchStateButton.generated.h"

/** Match-state transition this button requests from the game mode when triggered. */
UENUM(BlueprintType)
enum class EGeoMatchStateRequest : uint8
{
	StartMatch,
	WaitingToStart,
	WaitingPostMatch,
	/** No state transition: respawns the boss of every arena whose boss was defeated. */
	RespawnBosses
};

/** Floor pad requesting a match-state transition from the game mode. */
UCLASS()
class GEOTRINITY_API AGeoMatchStateButton : public AGeoFloorButton
{
	GENERATED_BODY()

protected:
	virtual void Press() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Match")
	EGeoMatchStateRequest StateRequest = EGeoMatchStateRequest::StartMatch;
};
