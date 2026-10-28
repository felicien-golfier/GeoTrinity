// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"

#include "GeoTestEnemy.generated.h"

/** Enemy whose body stays in the world once dead (no death visuals, no destroy), so a test can act on it again. */
UCLASS(NotPlaceable, Transient)
class AGeoTestBodyEnemy : public AEnemyCharacter
{
	GENERATED_BODY()

protected:
	virtual void DeathLogic() override {}
};
