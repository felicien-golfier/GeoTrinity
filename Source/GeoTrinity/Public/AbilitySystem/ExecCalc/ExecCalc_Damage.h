// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffectTypes.h"

#include "ExecCalc_Damage.generated.h"

/**
 * Execution calculation that applies IncomingDamage to the target.
 * Captures DamageMultiplier (source) and DamageReduction (target) attributes, and reads
 * SingleUseDamageMultiplier from FGeoGameplayEffectContext to support per-shot damage scaling.
 */
UCLASS()
class GEOTRINITY_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UExecCalc_Damage();

	virtual void Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	FGameplayEffectAttributeCaptureDefinition DamageMultiplierCaptureDef;
	FGameplayEffectAttributeCaptureDefinition DamageReductionCaptureDef;
};
