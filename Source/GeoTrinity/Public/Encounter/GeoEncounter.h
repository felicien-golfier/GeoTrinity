// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Encounter/GeoEncounterTypes.h"
#include "GameFramework/Actor.h"

#include "GeoEncounter.generated.h"

class APlayerStart;
class FDataValidationContext;
class UGeoEncounterAction;
class UGeoEncounterTrigger;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEncounterCompleted);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEncounterCounterChanged, FName /*Counter*/, int32 /*NewValue*/);

/**
 * AGeoEncounter's list of actions running one after the other: the one being executed is awaited before the next.
 * Reflected so the garbage collector nulls the payload's actor once it is destroyed, instead of leaving it dangling.
 */
USTRUCT()
struct FGeoEncounterActionRun
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UGeoEncounterAction>> Actions;

	UPROPERTY()
	FGeoEncounterPayload Payload;

	UPROPERTY()
	TObjectPtr<UGeoEncounterAction> Awaiting;

	/** The rule of the current step this run belongs to; INDEX_NONE for OnEnter and OnExit. */
	int32 RuleIndex = INDEX_NONE;
	int32 Next = 0;
	bool bActive = false;
	bool bAdvancing = false;
};

/**
 * A scripted sequence placed in the level: ordered Steps, each with rules (trigger + condition + actions). It enters a
 * step, arms its rules, runs the actions a firing selects one after the other, and moves on when a rule completes the
 * step or an action calls GoToStep. Server only; nothing replicates (Encounter/CLAUDE.md).
 *
 * Re-entrancy: a step change asked while the encounter is inside a call (arming, Execute, Fire, OnExit) is stored and
 * applied once the outermost call returns; the first request wins.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API AGeoEncounter : public AActor
{
	GENERATED_BODY()

public:
	AGeoEncounter();

	/** Server. Enters step 0 when the encounter is not running. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void Start();

	/** Server. Cancels everything and enters step 0. Valid in any state. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void Restart();

	/**
	 * Server. Runs OnSkipped of every encounter chained before this one, in chain order (each step's OnEnter then
	 * OnExit), then Start. Not on the chain: logged, started with nothing skipped (Encounter/CLAUDE.md, "Jump start").
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void StartAfterSkippingChain();

	/** Server. Leaves the current step (OnExit runs) for the named one; the current step's name re-enters it. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void GoToStep(FName StepName);

	/** Adds Delta to the step counter and tells OnCounterChanged. */
	UFUNCTION(BlueprintCallable, Category = "GeoEncounter")
	void AddToCounter(FName Counter, int32 Delta = 1);

	/** The step counter's value; 0 until written during this visit of the step. */
	UFUNCTION(BlueprintPure, Category = "GeoEncounter")
	int32 GetCounter(FName Counter) const { return Counters.FindRef(Counter); }

	/** Trigger only (UGeoEncounterTrigger::Fire). */
	void OnTriggerFired(UGeoEncounterTrigger const& Trigger, FGeoEncounterPayload const& Payload);

	/** Action only (UGeoEncounterAction::Finish). Ignored when no run awaits Action. */
	void OnActionFinished(UGeoEncounterAction const& Action);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Broadcast after the last step completes. */
	UPROPERTY(BlueprintAssignable, Category = "GeoEncounter")
	FOnEncounterCompleted OnCompleted;

	/** Broadcast on every AddToCounter. */
	FOnEncounterCounterChanged OnCounterChanged;

protected:
	virtual void BeginPlay() override;

	/** Stops the current step without OnExit (runs cancelled, triggers disarmed); no step change is accepted after. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter")
	bool bStartOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter")
	TArray<FGeoEncounterStep> Steps;

	/** The start of this encounter's room: a world URL portal equal to its PlayerStartTag, or to its number along
	 * the chain, starts the game here, earlier encounters skipped. */
	UPROPERTY(EditInstanceOnly, Category = "GeoEncounter")
	TObjectPtr<APlayerStart> JumpStart;

private:
	enum class EState : uint8
	{
		/** Not running: never started, completed, or ended. */
		Idle,
		Running,
		Exiting,
	};

	/** A step change waiting for the encounter to be outside every call. Target == Steps.Num() ends the encounter. */
	struct FPendingChange
	{
		int32 Target = INDEX_NONE;
		bool bRunExit = false;

		bool IsSet() const { return Target != INDEX_NONE; }
	};

	/** Counts the calls the encounter is inside; the outermost one applies the pending change when it ends. */
	struct FCallScope
	{
		explicit FCallScope(AGeoEncounter& InEncounter) : Encounter(InEncounter) { ++Encounter.CallDepth; }
		~FCallScope()
		{
			if (--Encounter.CallDepth == 0)
			{
				Encounter.ApplyPending();
			}
		}

		AGeoEncounter& Encounter;
	};

	void RequestChange(FPendingChange Change);
	void ApplyPending();
	/** Disarms every rule of the current step, then cancels every running action list: what a cancel causes (a kill)
	 * fires no rule of the step being left. */
	void StopCurrentStep();
	void EnterStep(int32 StepIndex);
	void DisarmRule(int32 RuleIndex);
	void StartRun(FGeoEncounterActionRun& Run, int32 RuleIndex, TArray<TObjectPtr<UGeoEncounterAction>> const& Actions,
				  FGeoEncounterPayload const& Payload);
	void Advance(FGeoEncounterActionRun& Run);
	void CancelRun(FGeoEncounterActionRun& Run);
	void OnRunFinished(FGeoEncounterActionRun const& Run);
	int32 FindStep(FName StepName) const;
	/** From the world URL's portal: a number N = the Nth encounter along the chain (0 = its start), else the encounter
	 * whose JumpStart tag is the portal; null when none matches. */
	AGeoEncounter* FindJumpTarget() const;
	/** The first encounter with bStartOnBeginPlay: where the chain begins; null when there is none. */
	AGeoEncounter* FindChainStart() const;
	/** Moves the first local pawn onto JumpStart (none: nothing to do). */
	void MovePlayerToJumpStart() const;
	/** Target of the first GeoStartEncounterAction of the steps, in any action list; null at the end of a chain. */
	AGeoEncounter* FindNextInChain() const;
	/** OnSkipped of every step's OnEnter then OnExit actions, in list order. Rule actions are never skipped. */
	void SkipActions() const;

	EState State = EState::Idle;
	int32 CurrentStep = INDEX_NONE;
	int32 ExitTarget = INDEX_NONE;
	int32 CallDepth = 0;
	FPendingChange Pending;

	TMap<FName, int32> Counters;

	/** Per rule of the current step. */
	TArray<bool> RuleArmed;
	UPROPERTY(Transient)
	TArray<FGeoEncounterActionRun> RuleRuns;
	UPROPERTY(Transient)
	FGeoEncounterActionRun EnterRun;
	UPROPERTY(Transient)
	FGeoEncounterActionRun ExitRun;
};
