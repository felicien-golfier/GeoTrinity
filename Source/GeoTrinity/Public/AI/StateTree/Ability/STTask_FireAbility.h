// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_FireAbility.generated.h"

/** Per-instance data for FSTTask_FireAbility (StateTree instance data pattern). */
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
struct GEOTRINITY_API FSTTask_FireAbility : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_FireProjectileAbilityInstanceData;

	FSTTask_FireAbility();

	virtual UStruct const* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	/** Activates the configured ability by tag on the pawn's ASC. Binds to OnAbilityEnded for async completion. Returns
	 * Running until the ability ends or failed if the ASC is missing or the tag is invalid. */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
										   FStateTreeTransitionResult const& Transition) const override;

	/** Removes the OnAbilityEnded delegate registered in EnterState. */
	virtual void ExitState(FStateTreeExecutionContext& Context,
						   FStateTreeTransitionResult const& Transition) const override;

private:
	UAbilitySystemComponent* GetASC(FStateTreeExecutionContext const& Context) const;
};
