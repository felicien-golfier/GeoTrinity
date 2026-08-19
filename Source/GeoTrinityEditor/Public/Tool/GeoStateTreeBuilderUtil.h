// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "GameplayTagContainer.h"
#include "StateTreeTypes.h"

#include "GeoStateTreeBuilderUtil.generated.h"

class UStateTree;
class UStateTreeState;
class UStateTreeEditorData;

/** Names resolving one enter-condition property binding to a Property Function output and its context input. */
USTRUCT(BlueprintType)
struct FGeoPropertyFunctionBinding
{
	GENERATED_BODY()

	/** Property on the condition struct to bind (e.g. "Left", "Right"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTrinity|Editor")
	FName ConditionPropertyName;

	/** Unqualified USTRUCT name of the Property Function (e.g. "FSTGetHealthRatioPropertyFunction"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTrinity|Editor")
	FName PropertyFunctionStructName;

	/** Output property on the function's InstanceData (e.g. "Output"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTrinity|Editor")
	FName FunctionOutputPropertyName;

	/** Input property on the function's InstanceData to bind to the context object (e.g. "Input"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTrinity|Editor")
	FName FunctionInputPropertyName;

	/** Unqualified UClass name of the context object to bind Input to (e.g. "GeoEnemyAIController"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTrinity|Editor")
	FName ContextClassName;
};

/**
 * Editor utility object for mutating UStateTree assets from Python or Blueprint automation.
 * Each method validates, compiles, and saves the asset atomically, so StateTree assets stay consistent
 * after every programmatic edit without requiring a manual recompile.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoStateTreeBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Adds a new state with one FSTTask_FireAbility task, compiles, and saves.
	 *
	 * @param ParentStateName  Name of the parent state; pass NAME_None to add at the root.
	 * @param InsertIndex      Position inside the parent's children; pass -1 to append.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFireAbilityStateByTagName(UStateTree* StateTree, FName StateName, FName AbilityTagName,
											 FName ParentStateName, int32 InsertIndex = -1);

	/**
	 * Adds one FSTTask_FireAbility to an existing state. A state runs all its tasks at once, so several of these
	 * fire their abilities together — SetTasksCompletion then decides whether the state waits for all of them.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFireAbilityTaskToState(UStateTree* StateTree, FName StateName, FName AbilityTagName);

	/**
	 * Adds a new empty state (no tasks) and compiles/saves. Use for idle/dormant states waiting on an OnEvent transition.
	 *
	 * @param ParentStateName  Name of the parent state; pass NAME_None to add at the root.
	 * @param InsertIndex      Position inside the parent's children; pass -1 to append.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddState(UStateTree* StateTree, FName StateName, FName ParentStateName, int32 InsertIndex = -1);

	/** Finds an existing state by name and replaces its FSTTask_FireAbility tag. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ReplaceFireAbilityTagInState(UStateTree* StateTree, FName StateName, FName NewAbilityTagName);

	/** Removes a state by name (searches recursively). Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void RemoveState(UStateTree* StateTree, FName StateName);

	/** Clears all transitions on a state. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ClearTransitions(UStateTree* StateTree, FName StateName);

	/**
	 * Adds a GotoState transition from SourceStateName to TargetStateName. Compiles and saves.
	 *
	 * @param Trigger       When the transition fires: OnStateSucceeded, OnStateFailed, OnStateCompleted, or OnEvent.
	 * @param EventTagName  Gameplay tag name that arms the transition; pass NAME_None for completion triggers.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddTransition(UStateTree* StateTree, FName SourceStateName, FName TargetStateName,
							  EStateTreeTransitionTrigger Trigger, FName EventTagName = NAME_None);

	/**
	 * Appends a FStateTreeCompareFloatCondition to a state's EnterConditions.
	 * Left must be bound to a Property Function output after calling BindConditionPropertyToPropertyFunction.
	 *
	 * @param Threshold  Right-hand threshold value to compare against.
	 * @param Operator   Comparison operator (Equal, NotEqual, Less, LessOrEqual, Greater, GreaterOrEqual).
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFloatEnterCondition(UStateTree* StateTree, FName StateName, float Threshold,
									   EGenericAICheck Operator, bool bInvert = false);

	/**
	 * Binds a property on an enter condition to a Property Function output and wires the function's Input to a context object.
	 *
	 * @param ConditionIndex  Zero-based index into the state's EnterConditions array.
	 * @param Binding         Names the condition property, the function struct, its output/input properties, and the context class.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BindConditionPropertyToPropertyFunction(UStateTree* StateTree, FName StateName, int32 ConditionIndex,
														FGeoPropertyFunctionBinding const& Binding);

	/**
	 * Adds a task of any type to an existing state with default instance data. Context properties (Category = Context)
	 * are auto-bound by the schema at compile; other instance data keeps its authored defaults. Compiles and saves.
	 *
	 * @param TaskStructName  Unqualified USTRUCT name of the task (e.g. "STTask_ChaseTarget").
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddTaskToState(UStateTree* StateTree, FName StateName, FName TaskStructName);

	/**
	 * Sets one property on a state task's instance data, importing Value the way the Details panel exports it.
	 * The one knob for tuning a task added by AddTaskToState, which otherwise leaves it on its struct defaults.
	 *
	 * @param TaskStructName  Unqualified USTRUCT name of the task to edit (e.g. "StateTreeDelayTask").
	 * @param PropertyName    Property on that task's instance data (e.g. "Duration").
	 * @param Value           New value in text form (e.g. "2.0").
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetTaskProperty(UStateTree* StateTree, FName StateName, FName TaskStructName, FName PropertyName,
								FString Value);

	/** Sets whether a state completes once every one of its tasks has completed, or as soon as any single one does. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetTasksCompletion(UStateTree* StateTree, FName StateName, EStateTreeTaskCompletionType Completion);

	/** Adds an FSTTask_SendEventAfterNCycles task to an existing state. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddSendEventAfterNCyclesTask(UStateTree* StateTree, FName StateName, int32 CyclesRequired,
											 FName EventTagName);

	/** Removes all enter conditions from a state. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ClearEnterConditions(UStateTree* StateTree, FName StateName);

	/** Sets the Required Event To Enter on a state. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetRequiredEventToEnter(UStateTree* StateTree, FName StateName, FName EventTagName);

	/** Clears the Required Event To Enter on a state. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ClearRequiredEventToEnter(UStateTree* StateTree, FName StateName);

	/** Logs all states recursively with indent and task tags. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ListStates(UStateTree* StateTree);

	/** Logs all enter conditions on a named state. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ListEnterConditions(UStateTree* StateTree, FName StateName);

private:
	/** Creates a state and inserts it under the named parent (NAME_None = root subtree). Returns the new state, or
	 * nullptr if the parent was not found. Does not compile/save â callers do that after any further setup. */
	static UStateTreeState* CreateAndInsertState(UStateTreeEditorData* EditorData, FName StateName,
												 FName ParentStateName, int32 InsertIndex);

	static void AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
									FName ParentStateName, int32 InsertIndex);
};
