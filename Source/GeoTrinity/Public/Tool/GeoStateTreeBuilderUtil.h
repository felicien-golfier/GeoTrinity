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

	/** Logs all states recursively with indent and task tags. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ListStates(UStateTree* StateTree);

private:
	static void AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
	                                FName ParentStateName, int32 InsertIndex);
};

#endif // WITH_EDITOR
