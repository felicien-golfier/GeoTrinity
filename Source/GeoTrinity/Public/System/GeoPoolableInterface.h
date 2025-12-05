// GeoPoolableInterface.h

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
	// Called when an actor is popped (acquired) from the pool
	virtual void Init() = 0;

	// Called when an actor is released back to the pool
	virtual void End() = 0;
};
