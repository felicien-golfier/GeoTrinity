// Copyright 2024 GeoTrinity. All Rights Reserved.

#if WITH_EDITOR

#include "Tool/GeoStateTreeBuilderUtil.h"

#include "AI/StateTree/Property/STPropertyFunction_GetHealthRatio.h"
#include "AI/StateTree/Ability/STTask_FireAbility.h"
#include "AI/StateTree/Utility/STTask_SendEventAfterNCycles.h"
#include "Conditions/StateTreeCommonConditions.h"
#include "FileHelpers.h"
#include "PropertyBindingPath.h"
#include "StateTree.h"
#include "StateTreeCompilerManager.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorPropertyBindings.h"
#include "StateTreeState.h"
#include "StateTreeTypes.h"

static UStateTreeState* FindStateRecursive(TArray<TObjectPtr<UStateTreeState>> const& States, FName StateName)
{
	for (UStateTreeState* State : States)
	{
		if (!State)
		{
			continue;
		}
		if (State->Name == StateName)
		{
			return State;
		}
		if (UStateTreeState* Found = FindStateRecursive(State->Children, StateName))
		{
			return Found;
		}
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
		{
			return true;
		}
	}
	return false;
}

static void CompileAndSave(UStateTree* StateTree, TCHAR const* CallerName)
{
	UStateTreeEditingSubsystem::ValidateStateTree(StateTree);
	bool const bSuccess = UE::StateTree::Compiler::FCompilerManager::CompileSynchronously(StateTree);
	ensureMsgf(bSuccess, TEXT("%s — StateTree compile failed for '%s'"), CallerName, *StateTree->GetName());
	UEditorLoadingAndSavingUtils::SavePackages({StateTree->GetPackage()}, false);
}

static void LogStatesRecursive(TArray<TObjectPtr<UStateTreeState>> const& States, int32 Depth)
{
	for (UStateTreeState const* State : States)
	{
		if (!State)
		{
			continue;
		}
		FString Indent = FString::ChrN(Depth * 2, ' ');
		FString TaskTags;
		for (FStateTreeEditorNode const& TaskNode : State->Tasks)
		{
			if (TaskNode.GetInstance().GetStruct()
				&& TaskNode.GetInstance().GetStruct()->IsChildOf(
					FSTTask_FireProjectileAbilityInstanceData::StaticStruct()))
			{
				FSTTask_FireProjectileAbilityInstanceData const* Data =
					TaskNode.GetInstance().GetPtr<FSTTask_FireProjectileAbilityInstanceData>();
				TaskTags += FString::Printf(TEXT(" [tag=%s]"), *Data->AbilityTag.ToString());
			}
		}
		if (State->SingleTask.GetInstance().GetStruct()
			&& State->SingleTask.GetInstance().GetStruct()->IsChildOf(
				FSTTask_FireProjectileAbilityInstanceData::StaticStruct()))
		{
			FSTTask_FireProjectileAbilityInstanceData const* Single =
				State->SingleTask.GetInstance().GetPtr<FSTTask_FireProjectileAbilityInstanceData>();
			TaskTags += FString::Printf(TEXT(" [singletask tag=%s]"), *Single->AbilityTag.ToString());
		}
		else if (UScriptStruct const* SingleStruct = Cast<UScriptStruct>(State->SingleTask.GetInstance().GetStruct()))
		{
			TaskTags += FString::Printf(TEXT(" [singletask=%s]"), *SingleStruct->GetName());
		}
		FString TransitionInfo;
		for (FStateTreeTransition const& T : State->Transitions)
		{
			FString TriggerStr = UEnum::GetValueAsString(T.Trigger);
			FString TypeStr = UEnum::GetValueAsString(T.State.LinkType);
			FString TargetStr =
				T.State.Name.IsNone() ? FString() : FString::Printf(TEXT("→'%s'"), *T.State.Name.ToString());
			TransitionInfo += FString::Printf(TEXT(" [%s %s%s]"), *TriggerStr, *TypeStr, *TargetStr);
		}
		UE_LOG(LogTemp, Warning, TEXT("StateTree state: %s'%s'%s%s"), *Indent, *State->Name.ToString(), *TaskTags,
			   *TransitionInfo);
		LogStatesRecursive(State->Children, Depth + 1);
	}
}

void UGeoStateTreeBuilderUtil::AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
												   FName ParentStateName, int32 InsertIndex)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	EditorData->Modify();

	if (ParentStateName.IsNone())
	{
		UStateTreeState& NewRootState = EditorData->AddSubTree(StateName);
		NewRootState.Modify();
		TStateTreeEditorNode<FSTTask_FireAbility>& TaskNode = NewRootState.AddTask<FSTTask_FireAbility>();
		TaskNode.GetInstance().GetMutablePtr<FSTTask_FireProjectileAbilityInstanceData>()->AbilityTag = AbilityTag;
		CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState"));
		return;
	}

	UStateTreeState* ParentState = FindStateRecursive(EditorData->SubTrees, ParentStateName);
	if (!ensureMsgf(ParentState, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState — parent state '%s' not found"),
					*ParentStateName.ToString()))
	{
		return;
	}

	ParentState->Modify();

	UStateTreeState* NewState = NewObject<UStateTreeState>(EditorData, StateName);
	NewState->Name = StateName;
	TStateTreeEditorNode<FSTTask_FireAbility>& TaskNode = NewState->AddTask<FSTTask_FireAbility>();
	TaskNode.GetInstance().GetMutablePtr<FSTTask_FireProjectileAbilityInstanceData>()->AbilityTag = AbilityTag;

	if (InsertIndex >= 0 && InsertIndex < ParentState->Children.Num())
	{
		ParentState->Children.Insert(NewState, InsertIndex);
	}
	else
	{
		ParentState->Children.Add(NewState);
	}

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityState"));
}

void UGeoStateTreeBuilderUtil::AddFireAbilityStateByTagName(UStateTree* StateTree, FName StateName,
															FName AbilityTagName, FName ParentStateName,
															int32 InsertIndex)
{
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
	ensureMsgf(Tag.IsValid(), TEXT("UGeoStateTreeBuilderUtil::AddFireAbilityStateByTagName — tag '%s' not found"),
			   *AbilityTagName.ToString());
	AddFireAbilityState(StateTree, StateName, Tag, ParentStateName, InsertIndex);
}

void UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState(UStateTree* StateTree, FName StateName,
															FName NewAbilityTagName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData,
					TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* TargetState = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(TargetState, TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(NewAbilityTagName, false);
	if (!ensureMsgf(NewTag.IsValid(),
					TEXT("UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState — tag '%s' not found"),
					*NewAbilityTagName.ToString()))
	{
		return;
	}

	TargetState->Modify();
	for (FStateTreeEditorNode& TaskNode : TargetState->Tasks)
	{
		if (TaskNode.GetInstance().GetStruct()
			&& TaskNode.GetInstance().GetStruct()->IsChildOf(FSTTask_FireProjectileAbilityInstanceData::StaticStruct()))
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
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::RemoveState — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	EditorData->Modify();
	bool const bFound = RemoveStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(bFound, TEXT("UGeoStateTreeBuilderUtil::RemoveState — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::RemoveState"));
}

void UGeoStateTreeBuilderUtil::ClearTransitions(UStateTree* StateTree, FName StateName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	EditorData->Modify();
	State->Modify();
	State->Transitions.Empty();
	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions"));
}

void UGeoStateTreeBuilderUtil::AddTransition(UStateTree* StateTree, FName SourceStateName, FName TargetStateName,
											 EStateTreeTransitionTrigger Trigger)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddTransition — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::AddTransition — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* SourceState = FindStateRecursive(EditorData->SubTrees, SourceStateName);
	if (!ensureMsgf(SourceState, TEXT("UGeoStateTreeBuilderUtil::AddTransition — source state '%s' not found"),
					*SourceStateName.ToString()))
	{
		return;
	}

	UStateTreeState* TargetState = FindStateRecursive(EditorData->SubTrees, TargetStateName);
	if (!ensureMsgf(TargetState, TEXT("UGeoStateTreeBuilderUtil::AddTransition — target state '%s' not found"),
					*TargetStateName.ToString()))
	{
		return;
	}

	EditorData->Modify();
	SourceState->Modify();
	SourceState->AddTransition(Trigger, EStateTreeTransitionType::GotoState, TargetState);
	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddTransition"));
}

void UGeoStateTreeBuilderUtil::AddFloatEnterCondition(UStateTree* StateTree, FName StateName, float Threshold,
													  EGenericAICheck Operator, bool bInvert)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	EditorData->Modify();
	State->Modify();

	TStateTreeEditorNode<FStateTreeCompareFloatCondition>& CondNode =
		State->AddEnterCondition<FStateTreeCompareFloatCondition>(Operator);
	CondNode.GetInstanceData().Right = Threshold;
	CondNode.GetNode().bInvert = bInvert;

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddFloatEnterCondition"));
}

void UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction(
	UStateTree* StateTree, FName StateName, int32 ConditionIndex, FName ConditionPropertyName,
	FName PropertyFunctionStructName, FName FunctionOutputPropertyName, FName FunctionInputPropertyName,
	FName ContextClassName)
{
	if (!ensureMsgf(StateTree,
					TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(
			EditorData,
			TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — EditorData is null on '%s'"),
			*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State,
					TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	if (!ensureMsgf(
			State->EnterConditions.IsValidIndex(ConditionIndex),
			TEXT(
				"UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — condition index %d out of range on state '%s'"),
			ConditionIndex, *StateName.ToString()))
	{
		return;
	}

	UScriptStruct const* FuncStruct = FindFirstObject<UScriptStruct>(*PropertyFunctionStructName.ToString());
	if (!ensureMsgf(FuncStruct,
					TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — struct '%s' not found"),
					*PropertyFunctionStructName.ToString()))
	{
		return;
	}

	UClass const* ContextClass = FindFirstObject<UClass>(*ContextClassName.ToString());
	if (!ensureMsgf(ContextClass,
					TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — class '%s' not found"),
					*ContextClassName.ToString()))
	{
		return;
	}

	FStateTreeEditorNode const& CondNode = State->EnterConditions[ConditionIndex];
	FPropertyBindingPath TargetPath(CondNode.ID, ConditionPropertyName);

	FPropertyBindingPath SourcePath = EditorData->GetPropertyEditorBindings()->AddFunctionBinding(
		FuncStruct, {FPropertyBindingPathSegment(FunctionOutputPropertyName)}, TargetPath);

	FStateTreeBindableStructDesc ContextDesc = EditorData->FindContextData(ContextClass, TEXT(""));
	if (ensureMsgf(
			ContextDesc.ID.IsValid(),
			TEXT(
				"UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction — class '%s' not found in StateTree context"),
			*ContextClassName.ToString()))
	{
		FPropertyBindingPath InputSourcePath(ContextDesc.ID);
		FPropertyBindingPath InputTargetPath(SourcePath.GetStructID(), FunctionInputPropertyName);
		EditorData->AddPropertyBinding(InputSourcePath, InputTargetPath);
	}

	EditorData->Modify();
	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction"));
}

void UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask(UStateTree* StateTree, FName StateName,
															int32 CyclesRequired, FName EventTagName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData,
					TEXT("UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(EventTagName, false);
	if (!ensureMsgf(EventTag.IsValid(),
					TEXT("UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask — tag '%s' not found"),
					*EventTagName.ToString()))
	{
		return;
	}

	EditorData->Modify();
	State->Modify();

	TStateTreeEditorNode<FSTTask_SendEventAfterNCycles>& TaskNode = State->AddTask<FSTTask_SendEventAfterNCycles>();
	FSTTask_SendEventAfterNCyclesInstanceData* Data =
		TaskNode.GetInstance().GetMutablePtr<FSTTask_SendEventAfterNCyclesInstanceData>();
	Data->CyclesRequired = CyclesRequired;
	Data->EventTag = EventTag;

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask"));
}

void UGeoStateTreeBuilderUtil::ClearEnterConditions(UStateTree* StateTree, FName StateName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearEnterConditions — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ClearEnterConditions — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::ClearEnterConditions — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	FStateTreeEditorPropertyBindings* Bindings = EditorData->GetPropertyEditorBindings();
	for (FStateTreeEditorNode const& CondNode : State->EnterConditions)
	{
		Bindings->RemoveBindings(FPropertyBindingPath(CondNode.ID),
								 FStateTreeEditorPropertyBindings::ESearchMode::Includes);
	}

	EditorData->Modify();
	State->Modify();
	State->EnterConditions.Empty();

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearEnterConditions"));
}

void UGeoStateTreeBuilderUtil::SetRequiredEventToEnter(UStateTree* StateTree, FName StateName, FName EventTagName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::SetRequiredEventToEnter — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::SetRequiredEventToEnter — EditorData is null on '%s'"),
					*StateTree->GetName()))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::SetRequiredEventToEnter — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(EventTagName, false);
	if (!ensureMsgf(EventTag.IsValid(), TEXT("UGeoStateTreeBuilderUtil::SetRequiredEventToEnter — tag '%s' not found"),
					*EventTagName.ToString()))
	{
		return;
	}

	EditorData->Modify();
	State->Modify();
	State->bHasRequiredEventToEnter = true;
	State->RequiredEventToEnter.Tag = EventTag;

	CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::SetRequiredEventToEnter"));
}

void UGeoStateTreeBuilderUtil::ListStates(UStateTree* StateTree)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ListStates — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ListStates — EditorData is null")))
	{
		return;
	}

	LogStatesRecursive(EditorData->SubTrees, 0);
}

void UGeoStateTreeBuilderUtil::ListEnterConditions(UStateTree* StateTree, FName StateName)
{
	if (!ensureMsgf(StateTree, TEXT("UGeoStateTreeBuilderUtil::ListEnterConditions — StateTree is null")))
	{
		return;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!ensureMsgf(EditorData, TEXT("UGeoStateTreeBuilderUtil::ListEnterConditions — EditorData is null")))
	{
		return;
	}

	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	if (!ensureMsgf(State, TEXT("UGeoStateTreeBuilderUtil::ListEnterConditions — state '%s' not found"),
					*StateName.ToString()))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("EnterConditions on state '%s': %d condition(s)"), *StateName.ToString(),
		   State->EnterConditions.Num());
	for (int32 i = 0; i < State->EnterConditions.Num(); ++i)
	{
		FStateTreeEditorNode const& Node = State->EnterConditions[i];
		UScriptStruct const* Struct = Cast<UScriptStruct>(Node.GetInstance().GetStruct());
		UE_LOG(LogTemp, Warning, TEXT("  [%d] struct=%s  ID=%s"), i, Struct ? *Struct->GetName() : TEXT("null"),
			   *Node.ID.ToString());
		if (Struct)
		{
			for (TFieldIterator<FProperty> It(Struct); It; ++It)
			{
				FString ValueStr;
				It->ExportText_InContainer(0, ValueStr, Node.GetInstance().GetMemory(), nullptr, nullptr, PPF_None);
				UE_LOG(LogTemp, Warning, TEXT("      %s = %s"), *It->GetName(), *ValueStr);
			}
		}
		FStateTreeEditorPropertyBindings const* Bindings = EditorData->GetPropertyEditorBindings();
		for (FStateTreePropertyPathBinding const& Binding : Bindings->GetBindings())
		{
			if (Binding.GetTargetPath().GetStructID() == Node.ID)
			{
				FPropertyBindingPath const& Src = Binding.GetSourcePath();
				FPropertyBindingPath const& Tgt = Binding.GetTargetPath();
				UE_LOG(LogTemp, Warning, TEXT("      binding: [%s].%s  ->  [%s].%s"), *Src.GetStructID().ToString(),
					   Src.NumSegments() > 0 ? *Src.GetSegment(0).GetName().ToString() : TEXT("?"),
					   *Tgt.GetStructID().ToString(),
					   Tgt.NumSegments() > 0 ? *Tgt.GetSegment(0).GetName().ToString() : TEXT("?"));
			}
		}
	}
}

#endif // WITH_EDITOR
