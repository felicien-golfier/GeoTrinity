// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "GeoGameInstance.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API UGeoGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;
};
