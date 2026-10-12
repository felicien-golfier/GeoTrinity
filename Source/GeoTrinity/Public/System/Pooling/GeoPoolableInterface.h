// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GeoPoolableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UGeoPoolableInterface : public UInterface
{
	GENERATED_BODY()
};

class GEOTRINITY_API IGeoPoolableInterface
{
	GENERATED_BODY()

public:
	/** Called when an actor is acquired from the pool. Reset state here before use. */
	virtual void Init() = 0;

	/** Called when an actor is returned to the pool. Deactivate visuals and clear state here. */
	virtual void End() = 0;
};
