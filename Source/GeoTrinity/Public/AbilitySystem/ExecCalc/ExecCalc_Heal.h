// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffectTypes.h"

#include "ExecCalc_Heal.generated.h"

/**
 * Execution calculation that applies IncomingHeal to the target.
 * Reads AppliedHealBoost (source) and ReceivedHealBoost (target) live from each side's stat ASC
 * (GeoASLib::GetStatAsc), so a deployable heals and is healed with its deployer's stats.
 */
UCLASS()
class GEOTRINITY_API UExecCalc_Heal : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	/** Scales IncomingHeal by both sides' heal boosts and the source's crit roll. */
	virtual void Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
