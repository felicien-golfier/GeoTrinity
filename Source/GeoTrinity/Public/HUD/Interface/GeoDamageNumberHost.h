// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GeoDamageNumberHost.generated.h"

UINTERFACE()
class UGeoDamageNumberHost : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by an actor that wants control over the floating combat numbers spawned from its attribute changes.
 * Actors that do not implement this interface show numbers; the HUD skips registration when ShowsDamageNumbers is false.
 */
class IGeoDamageNumberHost
{
	GENERATED_BODY()

public:
	/** Returns false to suppress the floating damage/heal numbers driven by this actor's Health/Shield changes. */
	virtual bool ShowsDamageNumbers() const = 0;
};
