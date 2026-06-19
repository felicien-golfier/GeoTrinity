// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "GeoCombattantWidgetHost.generated.h"

UINTERFACE()
class UGeoCombattantWidgetHost : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by the UI module's combatant widget component so gameplay can trigger its ASC bind without naming the
 * concrete UI type. Gameplay holds the component as an engine UWidgetComponent and calls through this interface.
 */
class IGeoCombattantWidgetHost
{
	GENERATED_BODY()

public:
	/** Binds the (already-created) widget to the owner's ASC if both exist. No-op otherwise. */
	virtual void BindToOwnerASC() const = 0;
};
