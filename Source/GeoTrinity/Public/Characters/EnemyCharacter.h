// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/GeoCharacter.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

#include "EnemyCharacter.generated.h"


class AGeoArena;
class UAnimMontage;
class UStateTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossDefeated);

/**
 * Enemy character controlled by a StateTree AI. Owns its ASC as a direct subobject (unlike
 * APlayableCharacter, where GAS lives on PlayerState). Boss death is handled in OnHealthChanged:
 * either resets health for looping attempts or broadcasts OnEnemyDefeated and self-destructs.
 */
UCLASS()
class GEOTRINITY_API AEnemyCharacter : public AGeoCharacter
{
	GENERATED_BODY()

public:
	/** Creates the ASC and attribute set as direct subobjects (enemies own their ASC, unlike players who use
	 * PlayerState). Sets AGeoEnemyAIController as the AI controller class. */
	AEnemyCharacter(FObjectInitializer const& ObjectInitializer);

	/** The tree the AI runs. Bosses share one base tree (the fight-start gate) and put their own spells in
	 * BehaviourStateTree. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacter|AI")
	TObjectPtr<UStateTree> StateTree;

	/** This enemy's own spell chain, run in StateTree's linked-asset state tagged AI.Boss.Behaviour. None = StateTree
	 * runs as authored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacter|AI")
	TObjectPtr<UStateTree> BehaviourStateTree;

	/** Played by AGeoArena::PlayIntro on its arena's first aggro, before the fight starts. None = no intro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCharacter|Boss")
	TObjectPtr<UAnimMontage> IntroMontage;

	UPROPERTY(BlueprintAssignable, Category = "GeoCharacter|Boss")
	FOnBossDefeated OnEnemyDefeated;

	UPROPERTY(BlueprintReadOnly, Category = "GeoCharacter|Boss")
	TWeakObjectPtr<AGeoArena> Arena;

	/** True while this enemy's arena runs a fight, false before aggro and for an enemy with no arena. Reads the
	 * arena's replicated flag, so every machine gets the same answer. */
	UFUNCTION(BlueprintPure, Category = "GeoBoss")
	bool IsFighting() const;

	/** Resets health to max and restarts the StateTree. Called on full-wipe to start a new attempt. */
	void ResetForNewAttempt();

protected:
	virtual void BeginPlay() override;

	/** Sets the combat level from the current GameState difficulty before calling the base GAS init. */
	virtual void InitGAS() override;

	/** Takes the enemy out of the fight — spawned elements, abilities, AI, movement, collision, damage — then lets the
	 * base play the death montage and destroy the actor once it has played out. */
	virtual void DeathLogic() override;

	/**
	 * Server-only gate on zero health: resets attributes to full when ResetToFullLifeWhenReachingZero is set,
	 * otherwise broadcasts OnEnemyDefeated and dies.
	 *
	 * @param NewValue  Current health value after the change.
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChanged(float NewValue);
	virtual void OnHealthChanged_Implementation(float NewValue);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "GeoCharacter|Boss")
	bool ResetToFullLifeWhenReachingZero = false;
};
