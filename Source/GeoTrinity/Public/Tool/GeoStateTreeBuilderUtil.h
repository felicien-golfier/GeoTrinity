// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "GameplayTagContainer.h"
#include "StateTreeTypes.h"

#include "GeoStateTreeBuilderUtil.generated.h"

class UStateTree;

UCLASS()
class GEOTRINITY_API UGeoStateTreeBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Adds a new state with one FSTTask_FireProjectileAbility task, compiles, and saves.
	 * ParentStateName — name of the parent state; pass NAME_None to add at root.
	 * InsertIndex     — position inside the parent's children; pass -1 to append.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFireAbilityStateByTagName(UStateTree* StateTree, FName StateName, FName AbilityTagName,
	                                         FName ParentStateName, int32 InsertIndex = -1);

	/** Finds an existing state by name and replaces its FSTTask_FireProjectileAbility tag. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ReplaceFireAbilityTagInState(UStateTree* StateTree, FName StateName, FName NewAbilityTagName);

	/** Removes a state by name (searches recursively). Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void RemoveState(UStateTree* StateTree, FName StateName);

	/** Clears all transitions on a state. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ClearTransitions(UStateTree* StateTree, FName StateName);

	/** Adds a GotoState transition from SourceStateName to TargetStateName. Trigger: OnStateSucceeded, OnStateFailed, or OnStateCompleted. Compiles and saves. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddTransition(UStateTree* StateTree, FName SourceStateName, FName TargetStateName, EStateTreeTransitionTrigger Trigger);

	/**
	 * Appends a FStateTreeCompareFloatCondition to a state's EnterConditions.
	 * Sets the Right (threshold) value and operator. Left must be bound manually in the editor.
	 * Operator: 0=Equal, 1=NotEqual, 2=Less, 3=LessOrEqual, 4=Greater, 5=GreaterOrEqual
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void AddFloatEnterCondition(UStateTree* StateTree, FName StateName, float Threshold, EGenericAICheck Operator, bool bInvert = false);

	/**
	 * Binds any property on a enter condition to a Property Function output, and binds the function's Input to a context object.
	 * ConditionIndex        — 0-based index into the state's EnterConditions array.
	 * ConditionPropertyName — property on the condition struct to bind (e.g. "Left", "Right").
	 * PropertyFunctionStructName — unqualified USTRUCT name of the Property Function (e.g. "FSTGetHealthRatioPropertyFunction").
	 * FunctionOutputPropertyName — output property on the function's InstanceData (e.g. "Output").
	 * FunctionInputPropertyName  — input property on the function's InstanceData to bind to context (e.g. "Input").
	 * ContextClassName      — unqualified UClass name of the context object to bind Input to (e.g. "GeoEnemyAIController").
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void BindConditionPropertyToPropertyFunction(UStateTree* StateTree, FName StateName, int32 ConditionIndex,
	                                                    FName ConditionPropertyName,
	                                                    FName PropertyFunctionStructName, FName FunctionOutputPropertyName,
	                                                    FName FunctionInputPropertyName, FName ContextClassName);

	/** Logs all states recursively with indent and task tags. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ListStates(UStateTree* StateTree);

private:
	static void AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
	                                FName ParentStateName, int32 InsertIndex);
};

#endif // WITH_EDITOR
