// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ExecCalc_Damage.generated.h"
#include "GameplayEffectExecutionCalculation.h"

/**
 * Modifies the manner Damage attribute is calculated for a GE
 */
UCLASS()
class GEOTRINITY_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UExecCalc_Damage();

	virtual void Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
