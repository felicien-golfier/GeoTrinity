// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoStateTreeBuilderUtil.h"

#include "AI/StateTree/Ability/STTask_FireAbility.h"
#include "AI/StateTree/Property/STPropertyFunction_GetHealthRatio.h"
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
#include "StateTreeTaskBase.h"
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
		if (!States[i])
		{
			continue;
		}
		if (States[i]->Name == StateName)
		{
			States.RemoveAt(i);
			return true;
		}
		if (RemoveStateRecursive(States[i]->Children, StateName))
		{
			return true;
		}
	}
	return false;
}

static void CompileAndSave(UStateTree* StateTree, ANSICHAR const* CallerName)
{
	UStateTreeEditingSubsystem::ValidateStateTree(StateTree);
	bool const bSuccess = UE::StateTree::Compiler::FCompilerManager::CompileSynchronously(StateTree);
	ensureMsgf(bSuccess, TEXT("%hs — StateTree compile failed for '%s'"), CallerName, *StateTree->GetName());
	UEditorLoadingAndSavingUtils::SavePackages({StateTree->GetPackage()}, false);
}

static UStateTreeEditorData* GetEditorData(UStateTree* StateTree, ANSICHAR const* CallerName)
{
	if (!ensureMsgf(StateTree, TEXT("%hs — StateTree is null"), CallerName))
	{
		return nullptr;
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	ensureMsgf(EditorData, TEXT("%hs — EditorData is null on '%s'"), CallerName, *StateTree->GetName());
	return EditorData;
}

static UStateTreeState* FindState(UStateTreeEditorData* EditorData, FName StateName, ANSICHAR const* CallerName)
{
	UStateTreeState* State = FindStateRecursive(EditorData->SubTrees, StateName);
	ensureMsgf(State, TEXT("%hs — state '%s' not found"), CallerName, *StateName.ToString());
	return State;
}

static bool ResolveTag(FName TagName, ANSICHAR const* CallerName, FGameplayTag& OutTag)
{
	OutTag = FGameplayTag::RequestGameplayTag(TagName, false);
	return ensureMsgf(OutTag.IsValid(), TEXT("%hs — gameplay tag '%s' not found"), CallerName, *TagName.ToString());
}

/**
 * Resolves editor data + the named state, brackets the edit in Modify(), and compiles+saves once Edit reports
 * success. Edit returning false aborts without compiling, so a failed validation leaves the asset untouched.
 */
static void WithState(UStateTree* StateTree, FName StateName, ANSICHAR const* CallerName,
					  TFunctionRef<bool(UStateTreeEditorData&, UStateTreeState&)> Edit)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, CallerName);
	if (!EditorData)
	{
		return;
	}

	UStateTreeState* State = FindState(EditorData, StateName, CallerName);
	if (!State)
	{
		return;
	}

	EditorData->Modify();
	State->Modify();
	if (!Edit(*EditorData, *State))
	{
		return;
	}

	CompileAndSave(StateTree, CallerName);
}

static void AddFireAbilityTask(UStateTreeState& State, FGameplayTag AbilityTag)
{
	TStateTreeEditorNode<FSTTask_FireAbility>& TaskNode = State.AddTask<FSTTask_FireAbility>();
	TaskNode.GetInstance().GetMutablePtr<FSTTask_FireAbilityInstanceData>()->AbilityTag = AbilityTag;
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
				&& TaskNode.GetInstance().GetStruct()->IsChildOf(FSTTask_FireAbilityInstanceData::StaticStruct()))
			{
				FSTTask_FireAbilityInstanceData const* Data =
					TaskNode.GetInstance().GetPtr<FSTTask_FireAbilityInstanceData>();
				TaskTags += FString::Printf(TEXT(" [tag=%s]"), *Data->AbilityTag.ToString());
			}
		}
		if (State->SingleTask.GetInstance().GetStruct()
			&& State->SingleTask.GetInstance().GetStruct()->IsChildOf(FSTTask_FireAbilityInstanceData::StaticStruct()))
		{
			FSTTask_FireAbilityInstanceData const* Single =
				State->SingleTask.GetInstance().GetPtr<FSTTask_FireAbilityInstanceData>();
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

UStateTreeState* UGeoStateTreeBuilderUtil::CreateAndInsertState(UStateTreeEditorData* EditorData, FName StateName,
																FName ParentStateName, int32 InsertIndex)
{
	EditorData->Modify();

	if (ParentStateName.IsNone())
	{
		UStateTreeState& NewRootState = EditorData->AddSubTree(StateName);
		NewRootState.Modify();
		return &NewRootState;
	}

	UStateTreeState* ParentState = FindStateRecursive(EditorData->SubTrees, ParentStateName);
	if (!ensureMsgf(ParentState, TEXT("%hs — parent state '%s' not found"), __FUNCTION__, *ParentStateName.ToString()))
	{
		return nullptr;
	}

	ParentState->Modify();

	UStateTreeState* NewState = NewObject<UStateTreeState>(EditorData, StateName);
	NewState->Name = StateName;

	if (InsertIndex >= 0 && InsertIndex < ParentState->Children.Num())
	{
		ParentState->Children.Insert(NewState, InsertIndex);
	}
	else
	{
		ParentState->Children.Add(NewState);
	}

	return NewState;
}

void UGeoStateTreeBuilderUtil::AddState(UStateTree* StateTree, FName StateName, FName ParentStateName,
										int32 InsertIndex)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, __FUNCTION__);
	if (!EditorData || !CreateAndInsertState(EditorData, StateName, ParentStateName, InsertIndex))
	{
		return;
	}

	CompileAndSave(StateTree, __FUNCTION__);
}

void UGeoStateTreeBuilderUtil::AddFireAbilityState(UStateTree* StateTree, FName StateName, FGameplayTag AbilityTag,
												   FName ParentStateName, int32 InsertIndex)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, __FUNCTION__);
	if (!EditorData)
	{
		return;
	}

	UStateTreeState* NewState = CreateAndInsertState(EditorData, StateName, ParentStateName, InsertIndex);
	if (!NewState)
	{
		return;
	}

	AddFireAbilityTask(*NewState, AbilityTag);

	CompileAndSave(StateTree, __FUNCTION__);
}

void UGeoStateTreeBuilderUtil::AddFireAbilityStateByTagName(UStateTree* StateTree, FName StateName,
															FName AbilityTagName, FName ParentStateName,
															int32 InsertIndex)
{
	FGameplayTag Tag;
	if (!ResolveTag(AbilityTagName, __FUNCTION__, Tag))
	{
		return;
	}
	AddFireAbilityState(StateTree, StateName, Tag, ParentStateName, InsertIndex);
}

void UGeoStateTreeBuilderUtil::AddFireAbilityTaskToState(UStateTree* StateTree, FName StateName, FName AbilityTagName)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(StateTree, StateName, Caller,
			  [AbilityTagName, Caller](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  FGameplayTag AbilityTag;
				  if (!ResolveTag(AbilityTagName, Caller, AbilityTag))
				  {
					  return false;
				  }

				  AddFireAbilityTask(State, AbilityTag);
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::ReplaceFireAbilityTagInState(UStateTree* StateTree, FName StateName,
															FName NewAbilityTagName)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(StateTree, StateName, Caller,
			  [NewAbilityTagName, Caller](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  FGameplayTag NewTag;
				  if (!ResolveTag(NewAbilityTagName, Caller, NewTag))
				  {
					  return false;
				  }

				  for (FStateTreeEditorNode& TaskNode : State.Tasks)
				  {
					  if (TaskNode.GetInstance().GetStruct()
						  && TaskNode.GetInstance().GetStruct()->IsChildOf(
							  FSTTask_FireAbilityInstanceData::StaticStruct()))
					  {
						  TaskNode.GetInstance().GetMutablePtr<FSTTask_FireAbilityInstanceData>()->AbilityTag = NewTag;
						  break;
					  }
				  }
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::RemoveState(UStateTree* StateTree, FName StateName)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, __FUNCTION__);
	if (!EditorData)
	{
		return;
	}

	EditorData->Modify();
	if (!ensureMsgf(RemoveStateRecursive(EditorData->SubTrees, StateName), TEXT("%hs — state '%s' not found"),
					__FUNCTION__, *StateName.ToString()))
	{
		return;
	}

	CompileAndSave(StateTree, __FUNCTION__);
}

void UGeoStateTreeBuilderUtil::ClearTransitions(UStateTree* StateTree, FName StateName)
{
	WithState(StateTree, StateName, __FUNCTION__,
			  [](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  State.Transitions.Empty();
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::AddTransition(UStateTree* StateTree, FName SourceStateName, FName TargetStateName,
											 EStateTreeTransitionTrigger Trigger, FName EventTagName)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, __FUNCTION__);
	if (!EditorData)
	{
		return;
	}

	UStateTreeState* SourceState = FindState(EditorData, SourceStateName, __FUNCTION__);
	UStateTreeState* TargetState = FindState(EditorData, TargetStateName, __FUNCTION__);
	if (!SourceState || !TargetState)
	{
		return;
	}

	EditorData->Modify();
	SourceState->Modify();

	if (Trigger == EStateTreeTransitionTrigger::OnEvent)
	{
		FGameplayTag EventTag;
		if (!ResolveTag(EventTagName, __FUNCTION__, EventTag))
		{
			return;
		}
		SourceState->AddTransition(Trigger, EventTag, EStateTreeTransitionType::GotoState, TargetState);
	}
	else
	{
		SourceState->AddTransition(Trigger, EStateTreeTransitionType::GotoState, TargetState);
	}

	CompileAndSave(StateTree, __FUNCTION__);
}

void UGeoStateTreeBuilderUtil::AddFloatEnterCondition(UStateTree* StateTree, FName StateName, float Threshold,
													  EGenericAICheck Operator, bool bInvert)
{
	WithState(StateTree, StateName, __FUNCTION__,
			  [Threshold, Operator, bInvert](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  TStateTreeEditorNode<FStateTreeCompareFloatCondition>& CondNode =
					  State.AddEnterCondition<FStateTreeCompareFloatCondition>(Operator);
				  CondNode.GetInstanceData().Right = Threshold;
				  CondNode.GetNode().bInvert = bInvert;
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::BindConditionPropertyToPropertyFunction(UStateTree* StateTree, FName StateName,
																	   int32 ConditionIndex,
																	   FGeoPropertyFunctionBinding const& Binding)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(
		StateTree, StateName, Caller,
		[StateName, ConditionIndex, &Binding, Caller](UStateTreeEditorData& EditorData, UStateTreeState& State)
		{
			if (!ensureMsgf(State.EnterConditions.IsValidIndex(ConditionIndex),
							TEXT("%hs — condition index %d out of range on state '%s'"), Caller, ConditionIndex,
							*StateName.ToString()))
			{
				return false;
			}

			UScriptStruct const* FuncStruct =
				FindFirstObject<UScriptStruct>(*Binding.PropertyFunctionStructName.ToString());
			UClass const* ContextClass = FindFirstObject<UClass>(*Binding.ContextClassName.ToString());
			if (!ensureMsgf(FuncStruct, TEXT("%hs — struct '%s' not found"), Caller,
							*Binding.PropertyFunctionStructName.ToString())
				|| !ensureMsgf(ContextClass, TEXT("%hs — class '%s' not found"), Caller,
							   *Binding.ContextClassName.ToString()))
			{
				return false;
			}

			FStateTreeEditorNode const& CondNode = State.EnterConditions[ConditionIndex];
			FPropertyBindingPath TargetPath(CondNode.ID, Binding.ConditionPropertyName);

			FPropertyBindingPath SourcePath = EditorData.GetPropertyEditorBindings()->AddFunctionBinding(
				FuncStruct, {FPropertyBindingPathSegment(Binding.FunctionOutputPropertyName)}, TargetPath);

			FStateTreeBindableStructDesc ContextDesc = EditorData.FindContextData(ContextClass, TEXT(""));
			if (ensureMsgf(ContextDesc.ID.IsValid(), TEXT("%hs — class '%s' not found in StateTree context"), Caller,
						   *Binding.ContextClassName.ToString()))
			{
				FPropertyBindingPath InputSourcePath(ContextDesc.ID);
				FPropertyBindingPath InputTargetPath(SourcePath.GetStructID(), Binding.FunctionInputPropertyName);
				EditorData.AddPropertyBinding(InputSourcePath, InputTargetPath);
			}
			return true;
		});
}

void UGeoStateTreeBuilderUtil::AddTaskToState(UStateTree* StateTree, FName StateName, FName TaskStructName)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(StateTree, StateName, Caller,
			  [TaskStructName, Caller](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  UScriptStruct const* TaskStruct = FindFirstObject<UScriptStruct>(*TaskStructName.ToString());
				  if (!ensureMsgf(TaskStruct && TaskStruct->IsChildOf(FStateTreeTaskBase::StaticStruct()),
								  TEXT("%hs — '%s' is not a StateTree task struct"), Caller,
								  *TaskStructName.ToString()))
				  {
					  return false;
				  }

				  FStateTreeEditorNode& TaskItem = State.Tasks.AddDefaulted_GetRef();
				  TaskItem.ID = FGuid::NewGuid();
				  TaskItem.Node.InitializeAs(TaskStruct);
				  FStateTreeNodeBase const& Node = TaskItem.Node.Get<FStateTreeNodeBase>();
				  if (UScriptStruct const* InstanceType = Cast<UScriptStruct const>(Node.GetInstanceDataType()))
				  {
					  TaskItem.Instance.InitializeAs(InstanceType);
				  }
				  if (UScriptStruct const* InstanceType = Cast<UScriptStruct const>(Node.GetExecutionRuntimeDataType()))
				  {
					  TaskItem.ExecutionRuntimeData.InitializeAs(InstanceType);
				  }
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::SetTaskProperty(UStateTree* StateTree, FName StateName, FName TaskStructName,
											   FName PropertyName, FString Value)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(StateTree, StateName, Caller,
			  [TaskStructName, PropertyName, Value, Caller, StateName](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  for (FStateTreeEditorNode& TaskNode : State.Tasks)
				  {
					  UScriptStruct const* NodeStruct = TaskNode.Node.GetScriptStruct();
					  UScriptStruct const* InstanceStruct = TaskNode.Instance.GetScriptStruct();
					  if (!NodeStruct || NodeStruct->GetFName() != TaskStructName || !InstanceStruct)
					  {
						  continue;
					  }

					  FProperty const* Property = InstanceStruct->FindPropertyByName(PropertyName);
					  if (!ensureMsgf(Property, TEXT("%hs — '%s' instance data has no property '%s'"), Caller,
									  *TaskStructName.ToString(), *PropertyName.ToString()))
					  {
						  return false;
					  }

					  Property->ImportText_InContainer(*Value, TaskNode.Instance.GetMutableMemory(), nullptr, PPF_None);
					  return true;
				  }

				  ensureMsgf(false, TEXT("%hs — state '%s' has no '%s' task"), Caller, *StateName.ToString(),
							 *TaskStructName.ToString());
				  return false;
			  });
}

void UGeoStateTreeBuilderUtil::SetTasksCompletion(UStateTree* StateTree, FName StateName,
												  EStateTreeTaskCompletionType Completion)
{
	WithState(StateTree, StateName, __FUNCTION__,
			  [Completion](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  State.TasksCompletion = Completion;
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::AddSendEventAfterNCyclesTask(UStateTree* StateTree, FName StateName,
															int32 CyclesRequired, FName EventTagName)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(StateTree, StateName, Caller,
			  [CyclesRequired, EventTagName, Caller](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  FGameplayTag EventTag;
				  if (!ResolveTag(EventTagName, Caller, EventTag))
				  {
					  return false;
				  }

				  TStateTreeEditorNode<FSTTask_SendEventAfterNCycles>& TaskNode =
					  State.AddTask<FSTTask_SendEventAfterNCycles>();
				  FSTTask_SendEventAfterNCyclesInstanceData* Data =
					  TaskNode.GetInstance().GetMutablePtr<FSTTask_SendEventAfterNCyclesInstanceData>();
				  Data->CyclesRequired = CyclesRequired;
				  Data->EventTag = EventTag;
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::ClearEnterConditions(UStateTree* StateTree, FName StateName)
{
	WithState(StateTree, StateName, __FUNCTION__,
			  [](UStateTreeEditorData& EditorData, UStateTreeState& State)
			  {
				  FStateTreeEditorPropertyBindings* Bindings = EditorData.GetPropertyEditorBindings();
				  for (FStateTreeEditorNode const& CondNode : State.EnterConditions)
				  {
					  Bindings->RemoveBindings(FPropertyBindingPath(CondNode.ID),
											   FStateTreeEditorPropertyBindings::ESearchMode::Includes);
				  }
				  State.EnterConditions.Empty();
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::SetRequiredEventToEnter(UStateTree* StateTree, FName StateName, FName EventTagName)
{
	ANSICHAR const* const Caller = __FUNCTION__;
	WithState(StateTree, StateName, Caller,
			  [EventTagName, Caller](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  FGameplayTag EventTag;
				  if (!ResolveTag(EventTagName, Caller, EventTag))
				  {
					  return false;
				  }

				  State.bHasRequiredEventToEnter = true;
				  State.RequiredEventToEnter.Tag = EventTag;
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::ClearRequiredEventToEnter(UStateTree* StateTree, FName StateName)
{
	WithState(StateTree, StateName, __FUNCTION__,
			  [](UStateTreeEditorData&, UStateTreeState& State)
			  {
				  State.bHasRequiredEventToEnter = false;
				  State.RequiredEventToEnter.Tag = FGameplayTag();
				  return true;
			  });
}

void UGeoStateTreeBuilderUtil::ListStates(UStateTree* StateTree)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, __FUNCTION__);
	if (!EditorData)
	{
		return;
	}

	LogStatesRecursive(EditorData->SubTrees, 0);
}

void UGeoStateTreeBuilderUtil::ListEnterConditions(UStateTree* StateTree, FName StateName)
{
	UStateTreeEditorData* EditorData = GetEditorData(StateTree, __FUNCTION__);
	if (!EditorData)
	{
		return;
	}

	UStateTreeState* State = FindState(EditorData, StateName, __FUNCTION__);
	if (!State)
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
