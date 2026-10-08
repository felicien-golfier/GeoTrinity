// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffectTypes.h"

#include "ExecCalc_Damage.generated.h"

/**
 * Execution calculation that applies IncomingDamage to the target.
 * Reads DamageMultiplier (source) and DamageReduction (target) live from each side's stat ASC (GeoASLib::GetStatAsc),
 * so a deployable fights with its deployer's stats, and reads SingleUseDamageMultiplier from FGeoGameplayEffectContext
 * to support per-shot damage scaling. A context with bSkipStatModifiers skips the two stats and the crit roll.
 */
UCLASS()
class GEOTRINITY_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	virtual void Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
