// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffectTypes.h"

#include "ExecCalc_Heal.generated.h"

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
