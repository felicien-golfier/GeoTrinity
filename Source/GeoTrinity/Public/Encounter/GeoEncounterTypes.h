// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoEncounterTypes.generated.h"

class AActor;
class UGeoEncounterAction;
class UGeoEncounterCondition;
class UGeoEncounterTrigger;

/** What a trigger reports about an occurrence. The trigger fills it, conditions and actions read it. */
USTRUCT(BlueprintType)
struct FGeoEncounterPayload
{
	GENERATED_BODY()

	/** The actor the event is about: who entered, who died. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	TObjectPtr<AActor> Actor = nullptr;

	/** The trigger's own running number since it was armed (entries so far, deaths so far). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	int32 Count = 0;

	/** A trigger-specific amount: a health ratio, characters still alive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	float Value = 0.f;

	/** The tag of a raised event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoEncounter")
	FGameplayTag Tag;
};

/** What a GeoCompareCondition reads. */
UENUM(BlueprintType)
enum class EGeoCompareSource : uint8
{
	PayloadCount,
	PayloadValue,
	Counter,
};

UENUM(BlueprintType)
enum class EGeoCompareOperator : uint8
{
	Equal,
	NotEqual,
	Less,
	LessOrEqual,
	Greater,
	GreaterOrEqual,
	MultipleOf,
};

/** One reaction of a step: when Trigger fires and Condition passes, Actions run in order. */
USTRUCT(BlueprintType)
struct FGeoEncounterRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "GeoEncounter")
	TObjectPtr<UGeoEncounterTrigger> Trigger;

	/** None passes every firing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "GeoEncounter")
	TObjectPtr<UGeoEncounterCondition> Condition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "GeoEncounter")
	TArray<TObjectPtr<UGeoEncounterAction>> Actions;

	/** False: the rule fires once per visit of the step, then its trigger is disarmed. True: every passing firing runs
	 *  the actions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter")
	bool bRepeat = false;

	/** The step completes once the actions have finished. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter")
	bool bCompletesStep = false;

	/** Step entered after the completion. None = the next one in the list. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter",
			  meta = (EditCondition = "bCompletesStep", EditConditionHides))
	FName StepAfter;
};

USTRUCT(BlueprintType)
struct FGeoEncounterStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter")
	FName Name;

	/** Run in order alongside the armed rules when the step is entered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "GeoEncounter")
	TArray<TObjectPtr<UGeoEncounterAction>> OnEnter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoEncounter")
	TArray<FGeoEncounterRule> Rules;

	/** Run in order, awaited, when the step completes or is left through GoToStep. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "GeoEncounter")
	TArray<TObjectPtr<UGeoEncounterAction>> OnExit;
};
