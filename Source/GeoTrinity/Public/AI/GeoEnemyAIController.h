// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"


class UGeoAIBlackboardComponent;
class UStateTree;
class UStateTreeAIComponent;

/**
 * AI controller for enemy characters. Starts a StateTree (via UStateTreeAIComponent) on possession
 * and leaves execution entirely to the tree tasks (FSTTask_FireProjectileAbility, etc.).
 * Also drives aggro detection (proximity + on-damage) and exposes RestartStateTree for wipe resets.
 */
UCLASS()
class GEOTRINITY_API AGeoEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UGeoAIBlackboardComponent> GeoBlackBoard;

	virtual void OnPossess(APawn* InPawn) override;

	UStateTreeAIComponent* GetStateTreeComp() const { return StateTreeComp; }

	/** Stops the StateTree logic and restarts it from the root (Dormant state). Resets aggro. */
	void RestartStateTree();

	UPROPERTY(EditAnywhere, Category = "Aggro")
	float AggroRadius = 1500.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

private:
	void CheckAggroDistance();

	UFUNCTION()
	void OnBossDamaged(float DamageAmount, FGameplayTag AbilityTag);

	void TriggerAggro();

	bool bAggroed = false;
	FTimerHandle AggroCheckTimer;
};
