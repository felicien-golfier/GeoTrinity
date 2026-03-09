// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Projectile/GeoProjectile.h"
#include "CoreMinimal.h"
#include "System/GeoPoolableInterface.h"

#include "GeoPooledProjectile.generated.h"

/**
 * Pooled projectile variant used for enemy patterns.
 * Non-replicated, managed by actor pooling subsystem.
 */
UCLASS()
class GEOTRINITY_API AGeoPooledProjectile
	: public AGeoProjectile
	, public IGeoPoolableInterface
{
	GENERATED_BODY()

public:
	AGeoPooledProjectile();

	// IGeoPoolableInterface
	virtual void End() override;
	virtual void Init() override;

protected:
	virtual void EndProjectileLife() override;
};
