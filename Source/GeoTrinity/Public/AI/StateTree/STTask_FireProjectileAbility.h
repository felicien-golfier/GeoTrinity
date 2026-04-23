// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_FireProjectileAbility.generated.h"

/** Per-instance data for FSTTask_FireProjectileAbility (StateTree instance data pattern). */
USTRUCT()
struct GEOTRINITY_API FSTTask_FireProjectileAbilityInstanceData
{
	GENERATED_BODY()

	/** The GameplayTag of the ability to activate on the pawn's ASC. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AbilityTag;

	/** Handle used to unbind the OnAbilityEnded delegate when the task exits. */
	FDelegateHandle AbilityEndedDelegateHandle;
};

/**
 * StateTree task that activates an ability by GameplayTag via the ASC
 */
USTRUCT(DisplayName = "Fire Projectile Ability", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_FireProjectileAbility : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_FireProjectileAbilityInstanceData;

	FSTTask_FireProjectileAbility();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
						   FStateTreeTransitionResult const& Transition) const override;

private:
	UAbilitySystemComponent* GetASC(FStateTreeExecutionContext const& Context) const;
};
