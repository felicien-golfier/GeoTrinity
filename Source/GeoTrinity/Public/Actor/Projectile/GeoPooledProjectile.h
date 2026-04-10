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

	/** Returns this projectile to the actor pool and disables collision. */
	virtual void End() override;
	/** Resets projectile state and re-enables collision after retrieval from the pool. */
	virtual void Init() override;

protected:
	/** Overrides the base class to release back to the pool instead of destroying the actor. */
	virtual void EndProjectileLife() override;
};
