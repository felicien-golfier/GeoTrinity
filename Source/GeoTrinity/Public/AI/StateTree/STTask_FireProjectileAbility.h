// STTask_FireProjectileAbility.h
#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"

#include "STTask_FireProjectileAbility.generated.h"

USTRUCT()
struct GEOTRINITY_API FSTTask_FireProjectileAbilityInstanceData
{
	GENERATED_BODY()

	// The GameplayTag of the ability to activate
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AbilityTag;

	// Delegate handle for cleanup
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
