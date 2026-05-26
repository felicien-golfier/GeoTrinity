// Copyright 2024 GeoTrinity. All Rights Reserved.

#if WITH_EDITOR

#include "Tool/GeoStateTreeBuilderUtil.h"

#include "AI/StateTree/STTask_FireProjectileAbility.h"
#include "AI/StateTree/STPropertyFunction_GetHealthRatio.h"
#include "FileHelpers.h"
#include "PropertyBindingPath.h"
#include "StateTree.h"
#include "StateTreeCompilerManager.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorPropertyBindings.h"
#include "StateTreeState.h"
#include "StateTreeTypes.h"
#include "Conditions/StateTreeCommonConditions.h"

static UStateTreeState* FindStateRecursive(const TArray<TObjectPtr<UStateTreeState>>& States, FName StateName)
{
	for (UStateTreeState* State : States)
	{
		if (!State) continue;
		if (State->Name == StateName) return State;
		if (UStateTreeState* Found = FindStateRecursive(State->Children, StateName))
			return Found;
	}
	return nullptr;
}

static bool RemoveStateRecursive(TArray<TObjectPtr<UStateTreeState>>& States, FName StateName)
{
	for (int32 i = 0; i < States.Num(); ++i)
	{
		if (States[i] && States[i]->Name == StateName)
		{
			States.RemoveAt(i);
			return true;
		}
		if (States[i] && RemoveStateRecursive(States[i]->Children, StateName))
			return true;
	}
	return false;
}

static void CompileAndSave(UStateTree* StateTree, const TCHAR* CallerName)
{
	UStateTreeEditingSubsystem::ValidateStateTree(StateTree);
	const bool bSuccess = UE::StateTree::Compiler::FCompilerManager::CompileSynchronously(StateTree);
	ensureMsgf(bSuccess, TEXT("%s — StateTree compile failed for '%s'"), CallerName, *StateTree->GetName());
	UEditorLoadingAndSavingUtils::SavePackages({ StateTree->GetPackage() }, false);
}

static void LogStatesRecursive(const TArray<TObjectPtr<UStateTreeState>>& States, int32 Depth)
{
	for (const UStateTreeState* State : States)
	{
		if (!State) continue;
		FString Indent = FString::ChrN(Depth * 2, ' ');
		FString TaskTags;
		for (const FStateTreeEditorNode& TaskNode : State->Tasks)
		{
			if (TaskNode.GetInstance().GetStruct() && TaskNode.GetInstance().GetStruct()->IsChildOf(FSTTask_FireProjectileAbilityInstanceData::StaticStruct()))
			{
				const FSTTask_FireProjectileAbilityInstanceData* Data = TaskNode.GetInstance().GetPtr<FSTTask_FireProjectileAbilityInstanceData>();
				TaskTags += FString::Printf(TEXT(" [tag=%s]"), *Data->AbilityTag.ToString());
			}
		}
		if (State->SingleTask.GetInstance().GetStruct() && State->SingleTask.GetInstance().GetStruct()->IsChildOf(FSTTask_FireProjectileAbilityInstanceData::StaticStruct()))
		{
			const FSTTask_FireProjectileAbilityInstanceData* Single = State->SingleTask.GetInstance().GetPtr<FSTTask_FireProjectileAbilityInstanceData>();
			TaskTags += FString::Printf(TEXT(" [singletask tag=%s]"), *Single->AbilityTag.ToString());
		}
		else if (const UScriptStruct* SingleStruct = Cast<UScriptStruct>(State->SingleTask.GetInstance().GetStruct()))
		{
			TaskTags += FString::Printf(TEXT(" [singletask=%s]"), *SingleStruct->GetName());
		}
		FString TransitionInfo;
		for (const FStateTreeTransition& T : State->Transitions)
		{
			FString TriggerStr = UEnum::GetValueAsString(T.Trigger);
			FString TypeStr = UEnum::GetValueAsString(T.State.LinkType);
			FString TargetStr = T.State.Name.IsNone() ? FString() : FString::Printf(TEXT("→'%s'"), *T.State.Name.ToString());
			TransitionInfo += FString::Printf(TEXT(" [%s %s%s]"), *TriggerStr, *TypeStr, *TargetStr);
		}
		UE_LOG(LogTemp, Warning, TEXT("StateTree state: %s'%s'%s%s"), *Indent, *State->Name.ToString(), *TaskTags, *TransitionInfo);
		LogStatesRecursive(State->Children, Depth + 1);
	}
}

void UGeoStateTreeBuilderUtil::AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
                                                    FName ParentStateName, int32 InsertIndex)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	EditorData->Modify();

	if (ParentStateName.IsNone())
	{
		UStateTreeState& NewRootState = EditorData->AddSubTree(StateName);
		NewRootState.Modify();
		TStateTreeEditorNode<FSTTask_FireProjectileAbility>& TaskNode = NewRootState.AddTask<FSTTask_FireProjectileAbility>();
		TaskNode.GetInstance().GetMutablePtr<FSTTask_FireProjectileAbilityInstanceData>()->AbilityTag = AbilityTag;
		CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState"));
		return;
	}

	UStateTreeState* ParentState = FindStateRecursive(EditorData->SubTrees, ParentStateName);
	if (!ensureMsgf(ParentState, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState — parent state '%s' not found"), *ParentStateName.ToString()))
		return;

	ParentState->Modify();

	UStateTreeState* NewState = NewObject<UStateTreeState>(EditorData, StateName);
	NewState->Name = StateName;
	TStateTreeEditorNode<FSTTask_FireProjectileAbility>& TaskNode = NewState->AddTask<FSTTask_FireProjectileAbility>();
	TaskNode.GetInstance().GetMutablePtr<FSTTask_FireProjectileAbilityInstanceData>()->AbilityTag = AbilityTag;

	if (InsertIndex >= 0 && InsertIndex < ParentState->Children.Num())
		ParentState->Children.Insert(NewState, InsertIndex);
	else
		ParentState->Children.Add(NewState);

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState"));
}

void UGeoStateTreeBuilderUtil::AddFireAbilityStateByTagName(UStateTree* StateTree, FName StateName, FName AbilityTagName,
                                                             FName ParentStateName, int32 InsertIndex)
{
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	ensureMsgf(Tag.IsValid(), TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityStateByTagName — tag '%s' not found"), *AbilityTagName.ToString());
	AddFireAbilityState(StateTree, StateName, Tag, ParentStateName, InsertIndex);
}

void UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState(UStateTree* StateTree, FName StateName, FName NewAbilityTagName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	UStateTreeState* TargetState = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(TargetState, TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — state '%s' not found"), *StateName.ToString()))
		return;

	FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(NewAbilityTagName, false);
	if (!ensureMsgf(NewTag.IsValid(), TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — tag '%s' not found"), *NewAbilityTagName.ToString()))
		return;

	TargetState->Modify();
	for (FStateTreeEditorNode& TaskNode : TargetState->Tasks)
	{
		if (TaskNode.GetInstance().GetStruct() && TaskNode.GetInstance().GetStruct()->IsChildOf(FSTTask_FireProjectileAbilityInstanceData::StaticStruct()))
		{
			TaskNode.GetInstance().GetMutablePtr<FSTTask_FireProjectileAbilityInstanceData>()->AbilityTag = NewTag;
			break;
		}
	}

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState"));
}

void UGeoStateTreeBuilderUtil::RemoveState(UStateTree* StateTree, FName StateName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::RemoveState — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::RemoveState — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	EditorData->Modify();
	const bool bFound = RemoveStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(bFound, TEXT("UGeoStateTreeBuilderUtil::RemoveState — state '%s' not found"), *StateName.ToString()))
		return;

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::RemoveState"));
}

void UGeoStateTreeBuilderUtil::ClearTransitions(UStateTree* StateTree, FName StateName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions — state '%s' not found"), *StateName.ToString()))
		return;

	EditorData->Modify();
	State->Modify();
	State->Transitions.Empty();
	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions"));
}

void UGeoStateTreeBuilderUtil::AddTransition(UStateTree* StateTree, FName SourceStateName, FName TargetStateName,
                                              EStateTreeTransitionTrigger Trigger)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddTransition — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::AddTransition — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	UStateTreeState* SourceState = FindStateRecursive(EditorData->SubTrees, SourceStateName);
	if (!ensureMsgf(SourceState, TEXT("UGeoStateTreeBuilderUtil::AddTransition — source state '%s' not found"), *SourceStateName.ToString()))
		return;

	UStateTreeState* TargetState = FindStateRecursive(EditorData->SubTrees, TargetStateName);
	if (!ensureMsgf(TargetState, TEXT("UGeoStateTreeBuilderUtil::AddTransition — target state '%s' not found"), *TargetStateName.ToString()))
		return;

	EditorData->Modify();
	SourceState->Modify();
	SourceState->AddTransition(Trigger, EStateTreeTransitionType::GotoState, TargetState);
	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddTransition"));
}

void UGeoStateTreeBuilderUtil::AddFloatEnterCondition(UStateTree* StateTree, FName StateName, float Threshold,
                                                       EGenericAICheck Operator, bool bInvert)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition — state '%s' not found"), *StateName.ToString()))
		return;

	EditorData->Modify();
	State->Modify();

	TStateTreeEditorNode<FStateTreeCompareFloatCondition>& CondNode = State->AddEnterCondition<FStateTreeCompareFloatCondition>(Operator);
	CondNode.GetInstanceData().Right = Threshold;
	CondNode.GetNode().bInvert = bInvert;

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition"));
}

void UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction(UStateTree* StateTree, FName StateName,
                                                                         int32 ConditionIndex,
                                                                         FName ConditionPropertyName,
                                                                         FName PropertyFunctionStructName,
                                                                         FName FunctionOutputPropertyName,
                                                                         FName FunctionInputPropertyName,
                                                                         FName ContextClassName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — EditorData is null on '%s'"), *StateTree->GetName()))
		return;

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — state '%s' not found"), *StateName.ToString()))
		return;

	if (!ensureMsgf(State->EnterConditions.IsValidIndex(ConditionIndex),
	                TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — condition index %d out of range on state '%s'"), ConditionIndex, *StateName.ToString()))
		return;

	const UScriptStruct* FuncStruct = FindFirstObject<UScriptStruct>(*PropertyFunctionStructName.ToString());
	if (!ensureMsgf(FuncStruct, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — struct '%s' not found"), *PropertyFunctionStructName.ToString()))
		return;

	const UClass* ContextClass = FindFirstObject<UClass>(*ContextClassName.ToString());
	if (!ensureMsgf(ContextClass, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — class '%s' not found"), *ContextClassName.ToString()))
		return;

	const FStateTreeEditorNode& CondNode = State->EnterConditions[ConditionIndex];
	FPropertyBindingPath TargetPath(CondNode.ID, ConditionPropertyName);

	FPropertyBindingPath SourcePath = EditorData->GetPropertyEditorBindings()->AddFunctionBinding(
		FuncStruct,
		{ FPropertyBindingPathSegment(FunctionOutputPropertyName) },
		TargetPath);

	FStateTreeBindableStructDesc ContextDesc = EditorData->FindContextData(ContextClass, TEXT(""));
	if (ensureMsgf(ContextDesc.ID.IsValid(), TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — class '%s' not found in StateTree context"), *ContextClassName.ToString()))
	{
		FPropertyBindingPath InputSourcePath(ContextDesc.ID);
		FPropertyBindingPath InputTargetPath(SourcePath.GetStructID(), FunctionInputPropertyName);
		EditorData->AddPropertyBinding(InputSourcePath, InputTargetPath);
	}

	EditorData->Modify();
	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction"));
}

void UGeoStateTreeBuilderUtil::ListStates(UStateTree* StateTree)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ListStates — StateTree is null")))
		return;

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ListStates — EditorData is null")))
		return;

	LogStatesRecursive(EditorData->SubTrees, 0);
}

#endif // WITH_EDITOR
