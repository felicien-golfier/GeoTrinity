// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Engine/TargetPoint.h"
#include "GameplayTagContainer.h"

#include "GeoTargetPoint.generated.h"

/** ATargetPoint extended with a GameplayTagContainer for GeoLib::GetTargetPoints filtering. */
UCLASS()
class GEOTRINITY_API AGeoTargetPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTargetPoint")
	FGameplayTagContainer GameplayTags;
};
