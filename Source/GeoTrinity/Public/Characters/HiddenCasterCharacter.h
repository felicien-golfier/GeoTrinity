// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"

#include "HiddenCasterCharacter.generated.h"

/**
 * Enemy with nothing on screen and nothing to hit. Lends an ASC to a room with no boss so an encounter can cast hazard
 * patterns and waves through it (UGeoActivateAbilityAction). A Blueprint child lists the abilities it can cast.
 */
UCLASS()
class GEOTRINITY_API AHiddenCasterCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	/** Skips the health bar, hides the actor. */
	AHiddenCasterCharacter(FObjectInitializer const& ObjectInitializer);

	/** Never aggroed: aggro would start a fight and make it touchable again. */
	virtual bool CanBeAggroed() const override { return false; }

protected:
	/** Server: untouchable from the start, so it never dies. */
	virtual void BeginPlay() override;
};
