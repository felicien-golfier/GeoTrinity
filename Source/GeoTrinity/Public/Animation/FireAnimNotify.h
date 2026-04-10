// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"

#include "FireAnimNotify.generated.h"

/**
 * Animation notify fired at the moment a weapon fires.
 * Used to synchronize fire audio/VFX with the animation frame.
 */
UCLASS()
class GEOTRINITY_API UFireAnimNotify : public UAnimNotify
{
	GENERATED_BODY()
};
