// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"

#include "GeoEnemyAIController.generated.h"


class APlayableCharacter;
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
	/** Disables control rotation sync from pawn orientation so the base AAIController::Tick cannot override the character's clamped turn toward TargetYaw. */
	AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer = FObjectInitializer::Get());

	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(FGenericTeamId const& NewTeamId) override;

	/** Returns the team ID used for attitude queries and team-based targeting. */
	virtual FGenericTeamId GetGenericTeamId() const override;

	/**
	 * Starts the proximity aggro timer and binds the damage-received delegate on the given enemy.
	 * Once called, the AI will begin targeting the nearest player and immediately aggro on any incoming hit.
	 *
	 * @param EnemyChar   The possessed enemy whose ASC receives the on-damage aggro binding.
	 */
	void InitializeAggro(AEnemyCharacter const* EnemyChar);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UGeoAIBlackboardComponent> GeoBlackBoard;

	/** Initializes the StateTree component and begins aggro detection on the newly possessed enemy pawn. */
	virtual void OnPossess(APawn* InPawn) override;
	/** Clears aggro state and stops the StateTree when the pawn is unpossessed. */
	virtual void OnUnPossess() override;
	/** Updates the current aggro target each frame based on proximity and the target-switch delay. */
	virtual void Tick(float DeltaTime) override;
	/** Returns the StateTree AI component that drives this controller's behavior logic. */
	UStateTreeAIComponent* GetStateTreeComp() const { return StateTreeComp; }

	/** Stops the StateTree logic and restarts it from the root (Dormant state). Resets aggro. */
	void ResetAI();

	UPROPERTY(EditAnywhere, Category = "Aggro")
	float AggroRadius = 1500.f;

	/** Seconds a closer player must remain the closest before target switches to them. */
	UPROPERTY(EditAnywhere, Category = "Aggro")
	float TargetSwitchDelay = 1.f;

	/** Recomputed every Tick by UpdateCurrentTarget(); read by StateTree movement/facing tasks instead of each
	 * resolving a target themselves. */
	APlayableCharacter* GetCurrentTarget() const { return CurrentTarget; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComp;

private:
	void CheckAggroDistance();
	/** Targets the nearest live player; a different player must stay the nearest for TargetSwitchDelay seconds
	 * before it replaces the current target. */
	void UpdateCurrentTarget(float DeltaTime);

	UFUNCTION()
	void OnGEApplied(UAbilitySystemComponent* Source, FGameplayEffectSpec const& Spec,
					 FActiveGameplayEffectHandle Handle);

	void ClearAggro();
	void InitializeStateTree(AEnemyCharacter const* EnemyChar) const;

	void TriggerAggro();

	bool bAggroed = false;
	FTimerHandle AggroCheckTimer;

	UPROPERTY(Transient)
	TObjectPtr<APlayableCharacter> CurrentTarget;

	UPROPERTY(Transient)
	TObjectPtr<APlayableCharacter> PendingTarget;

	float PendingTargetElapsedTime = 0.f;
};
