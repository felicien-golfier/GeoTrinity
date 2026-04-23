// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffectTypes.h"

#include "ExecCalc_Heal.generated.h"

/**
 * Execution calculation that applies IncomingHeal to the target.
 * Captures HealMultiplier from the source and broadcasts OnHealProvided on the source ASC
 * unless bSuppressHealProvided is set on the effect context.
 */
UCLASS()
class GEOTRINITY_API UExecCalc_Heal : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UExecCalc_Heal();

	virtual void Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	FGameplayEffectAttributeCaptureDefinition HealMultiplierCaptureDef;
};
