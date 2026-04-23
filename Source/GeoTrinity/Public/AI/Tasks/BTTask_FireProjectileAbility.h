// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "CoreMinimal.h"

#include "BTTask_FireProjectileAbility.generated.h"

/**
 * Behavior tree task that activates an ability by GameplayTag via the pawn's ASC.
 * The ability tag is read from a blackboard key. Task succeeds immediately after activation.
 * @note Prefer the StateTree equivalent (FSTTask_FireProjectileAbility) for new AI behavior.
 */
UCLASS(DisplayName = "Fire Projectile Ability")
class GEOTRINITY_API UBTTask_FireProjectileAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FireProjectileAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// Optional: if set, the task will try to activate abilities matching this gameplay tag via ASC.
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AbilityTagKey;
};
