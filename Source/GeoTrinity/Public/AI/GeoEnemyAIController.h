// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"


class UGeoAIBlackboardComponent;
class UStateTree;
class UStateTreeAIComponent;

/**
 * AI controller for enemy characters. Starts a StateTree (via UStateTreeAIComponent) on possession
 * and leaves execution entirely to the tree tasks (FSTTask_FireProjectileAbility, etc.).
 * Also drives aggro detection (proximity + on-damage) and exposes ResetAI for wipe resets.
 */
UCLASS()
class GEOTRINITY_API AGeoEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(FGenericTeamId const& NewTeamId) override;

	virtual FGenericTeamId GetGenericTeamId() const override;
	void InitializeAggro(AEnemyCharacter const* EnemyChar);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UGeoAIBlackboardComponent> GeoBlackBoard;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	UStateTreeAIComponent* GetStateTreeComp() const { return StateTreeComp; }

	/** Stops the StateTree logic and restarts it from the root (Dormant state). Resets aggro. */
	void ResetAI();

	UPROPERTY(EditAnywhere, Category = "Aggro")
	float AggroRadius = 1500.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

private:
	void CheckAggroDistance();

	UFUNCTION()
	void OnGEApplied(UAbilitySystemComponent* Source, FGameplayEffectSpec const& Spec,
					 FActiveGameplayEffectHandle Handle);

	void ClearAggro();
	void InitializeStateTree(AEnemyCharacter const* EnemyChar) const;

	void TriggerAggro();

	bool bAggroed = false;
	FTimerHandle AggroCheckTimer;
};
