// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/Action/Cat/GeoCatAction.h"

#include "GeoCatWalkAction.generated.h"

/** The Cat walks through Path in order, stopping StopDistance short of the last point, facing it. */
UCLASS()
class GEOTRINITY_API UGeoCatWalkAction : public UGeoCatAction
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Level actors (target points, a pad), read when the order is given. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TArray<TObjectPtr<AActor>> Path;

	/** cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "1"))
	float Speed = 150.f;

	/** How far before the last point the walk ends (a portal clip opening ahead of the cat). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter", meta = (ClampMin = "0"))
	float StopDistance = 0.f;

protected:
	virtual void GiveOrder(AGeoCat& OrderedCat) const override;
};
