// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "GeoGameInstance.generated.h"

/**
 * Custom game instance for GeoTrinity.
 * Performs one-time initialization of global systems (e.g. native gameplay tags) on game start.
 */
UCLASS()
class GEOTRINITY_API UGeoGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	/** Initializes native gameplay tags and other global systems. */
	virtual void Init() override;
};
