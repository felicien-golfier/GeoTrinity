// STTask_FireProjectileAbility.h
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"

#include "STTask_FireProjectileAbility.generated.h"

USTRUCT()
struct GEOTRINITY_API FSTTask_FireProjectileAbilityInstanceData
{
	GENERATED_BODY()

	// The GameplayTag of the ability to activate
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AbilityTag;
};

/**
 * StateTree task that activates an ability by GameplayTag via the ASC
 */
USTRUCT(DisplayName = "Fire Projectile Ability", Category = "GeoTrinity|AI")
struct GEOTRINITY_API FSTTask_FireProjectileAbility : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_FireProjectileAbilityInstanceData;

	FSTTask_FireProjectileAbility() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
