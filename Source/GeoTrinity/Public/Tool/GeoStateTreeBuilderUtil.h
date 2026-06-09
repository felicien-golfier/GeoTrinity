// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "GameplayTagContainer.h"
#include "StateTreeTypes.h"

#include "GeoStateTreeBuilderUtil.generated.h"

class UStateTree;
class UStateTreeState;
class UStateTreeEditorData;

UCLASS()
class GEOTRINITY_API UGeoStateTreeBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Adds a new state with one FSTTask_FireAbility task, compiles, and saves.
	 * ParentStateName — name of the parent state; pass NAME_None to add at root.
	 * InsertIndex     — position inside the parent's children; pass -1 to append.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFireAbilityStateByTagName(UStateTree* StateTree, FName StateName, FName AbilityTagName,
											 FName ParentStateName, int32 InsertIndex = -1);

	/**
	 * Adds a new empty state (no tasks) and compiles/saves. Use for idle/dormant states that wait on an OnEvent
	 * transition. ParentStateName — name of the parent; pass NAME_None to add at root. InsertIndex — position in the
	 * parent's children; pass -1 to append.
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

	/** Adds a GotoState transition from SourceStateName to TargetStateName. Trigger: OnStateSucceeded, OnStateFailed,
	 * OnStateCompleted, or OnEvent. For OnEvent transitions pass the event tag name in EventTagName; leave it None for
	 * completion triggers. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddTransition(UStateTree* StateTree, FName SourceStateName, FName TargetStateName,
							  EStateTreeTransitionTrigger Trigger, FName EventTagName = NAME_None);

	/**
	 * Appends a FStateTreeCompareFloatCondition to a state's EnterConditions.
	 * Sets the Right (threshold) value and operator. Left must be bound manually in the editor.
	 * Operator: 0=Equal, 1=NotEqual, 2=Less, 3=LessOrEqual, 4=Greater, 5=GreaterOrEqual
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFloatEnterCondition(UStateTree* StateTree, FName StateName, float Threshold,
									   EGenericAICheck Operator, bool bInvert = false);

	/**
	 * Binds any property on a enter condition to a Property Function output, and binds the function's Input to a
	 * context object. ConditionIndex        — 0-based index into the state's EnterConditions array.
	 * ConditionPropertyName — property on the condition struct to bind (e.g. "Left", "Right").
	 * PropertyFunctionStructName — unqualified USTRUCT name of the Property Function (e.g.
	 * "FSTGetHealthRatioPropertyFunction"). FunctionOutputPropertyName — output property on the function's InstanceData
	 * (e.g. "Output"). FunctionInputPropertyName  — input property on the function's InstanceData to bind to context
	 * (e.g. "Input"). ContextClassName      — unqualified UClass name of the context object to bind Input to (e.g.
	 * "GeoEnemyAIController").
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BindConditionPropertyToPropertyFunction(UStateTree* StateTree, FName StateName, int32 ConditionIndex,
														FName ConditionPropertyName, FName PropertyFunctionStructName,
														FName FunctionOutputPropertyName,
														FName FunctionInputPropertyName, FName ContextClassName);

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
	 * nullptr if the parent was not found. Does not compile/save — callers do that after any further setup. */
	static UStateTreeState* CreateAndInsertState(UStateTreeEditorData* EditorData, FName StateName,
												 FName ParentStateName, int32 InsertIndex);

	static void AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
									FName ParentStateName, int32 InsertIndex);
};

#endif // WITH_EDITOR
