// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"

#include "HiddenCasterCharacter.generated.h"

/**
 * Enemy with nothing on screen and nothing to hit. Lends an ASC to the encounters so they can cast hazard patterns and
 * waves through it (UGeoEncounterSubsystem::GetSharedCaster, UGeoActivateAbilityAction).
 */
UCLASS()
class GEOTRINITY_API AHiddenCasterCharacter : public AEnemyCharacter
{
	GENERATED_BODY()

public:
	/** Skips the health bar; DefaultAttributes = the empty UGameplayEffect (no stats yet, nothing to fill per
	 * Blueprint). */
	AHiddenCasterCharacter(FObjectInitializer const& ObjectInitializer);

	/** Never aggroed: aggro would start a fight and make it touchable again. */
	virtual bool CanBeAggroed() const override { return false; }

protected:
	virtual bool NeedsCombattantWidget() const override { return false; }

	/** Server: untouchable from the start, so it never dies. */
	virtual void BeginPlay() override;
};
