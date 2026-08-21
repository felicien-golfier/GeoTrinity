// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoNiagaraBuilderUtil.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "FileHelpers.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"

/** One resolved target: the system owning the edit, the emitter's handle and the stage script it addresses. */
struct FGeoNiagaraStage
{
	UNiagaraSystem* System = nullptr;
	FNiagaraEmitterHandle* Handle = nullptr;
	UNiagaraScript* Script = nullptr;

	bool IsValid() const { return Script != nullptr; }
};

static UNiagaraSystem* LoadNiagaraSystem(FString const& SystemPath)
{
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *SystemPath);
	ensureMsgf(System, TEXT("GeoNiagaraBuilderUtil: no NiagaraSystem at '%s'"), *SystemPath);
	return System;
}

static FGeoNiagaraStage FindStage(FString const& SystemPath, FName EmitterName, ENiagaraScriptUsage Usage)
{
	FGeoNiagaraStage Stage;
	Stage.System = LoadNiagaraSystem(SystemPath);
	if (!Stage.System)
	{
		return Stage;
	}

	for (FNiagaraEmitterHandle& Handle : Stage.System->GetEmitterHandles())
	{
		if (Handle.GetName() == EmitterName)
		{
			Stage.Handle = &Handle;
			break;
		}
	}
	if (!ensureMsgf(Stage.Handle, TEXT("GeoNiagaraBuilderUtil: no emitter '%s' in '%s'"), *EmitterName.ToString(),
					*SystemPath))
	{
		return Stage;
	}

	FVersionedNiagaraEmitterData const* EmitterData = Stage.Handle->GetInstance().GetEmitterData();
	Stage.Script = EmitterData ? EmitterData->GetScript(Usage, FGuid()) : nullptr;
	ensureMsgf(Stage.Script, TEXT("GeoNiagaraBuilderUtil: emitter '%s' has no script for usage %d"),
			   *EmitterName.ToString(), static_cast<int32>(Usage));
	return Stage;
}

static void CollectFunctionNodes(UNiagaraScript& Script, ENiagaraScriptUsage Usage,
								 TArray<UNiagaraNodeFunctionCall*>& OutNodes)
{
	UNiagaraNodeOutput* OutputNode = FNiagaraEditorUtilities::GetScriptOutputNode(Script);
	UEdGraph* Graph = OutputNode ? OutputNode->GetGraph() : nullptr;
	if (!ensureMsgf(Graph, TEXT("GeoNiagaraBuilderUtil: no stack graph for usage %d"), static_cast<int32>(Usage)))
	{
		return;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UNiagaraNodeFunctionCall* FunctionNode = Cast<UNiagaraNodeFunctionCall>(Node);
		if (FunctionNode && FNiagaraStackGraphUtilities::GetOutputNodeUsage(*FunctionNode) == Usage)
		{
			OutNodes.Add(FunctionNode);
		}
	}
}

static UNiagaraNodeFunctionCall* FindFunctionNode(UNiagaraScript& Script, ENiagaraScriptUsage Usage,
												  FName FunctionName)
{
	TArray<UNiagaraNodeFunctionCall*> FunctionNodes;
	CollectFunctionNodes(Script, Usage, FunctionNodes);

	FString const TargetName = FunctionName.ToString();
	for (UNiagaraNodeFunctionCall* FunctionNode : FunctionNodes)
	{
		if (FunctionNode->GetFunctionName() == TargetName)
		{
			return FunctionNode;
		}
	}
	ensureMsgf(false, TEXT("GeoNiagaraBuilderUtil: no function '%s' in usage %d"), *TargetName,
			   static_cast<int32>(Usage));
	return nullptr;
}

/** Only static switches surface as pins on a function call node; every other input is graph-internal. */
static UEdGraphPin* FindSwitchPin(UNiagaraNodeFunctionCall& FunctionNode, FName SwitchName)
{
	FName const NamespacedName = FName(*FString::Printf(TEXT("Module.%s"), *SwitchName.ToString()));
	for (UEdGraphPin* Pin : FunctionNode.Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && (Pin->PinName == SwitchName || Pin->PinName == NamespacedName))
		{
			return Pin;
		}
	}
	ensureMsgf(false, TEXT("GeoNiagaraBuilderUtil: function '%s' has no switch pin '%s'"),
			   *FunctionNode.GetFunctionName(), *SwitchName.ToString());
	return nullptr;
}

static FName MakeConstantName(FGeoNiagaraStage const& Stage, ENiagaraScriptUsage Usage, FName FunctionName,
							  FName InputName)
{
	FName const AliasedInput = FName(*FString::Printf(TEXT("%s.%s"), *FunctionName.ToString(), *InputName.ToString()));
	return FName(*FNiagaraUtilities::CreateRapidIterationConstantName(AliasedInput,
																	  *Stage.Handle->GetUniqueInstanceName(), Usage));
}

/**
 * A value input's type lives on the rapid-iteration constant the compiler emits for it, not on any pin, so a
 * freshly added function must have compiled once before its inputs can be addressed.
 */
static bool FindInputConstant(FGeoNiagaraStage const& Stage, ENiagaraScriptUsage Usage, FName FunctionName,
							  FName InputName, FNiagaraVariable& OutConstant)
{
	FName const ConstantName = MakeConstantName(Stage, Usage, FunctionName, InputName);
	for (FNiagaraVariableWithOffset const& Constant : Stage.Script->RapidIterationParameters.ReadParameterVariables())
	{
		if (Constant.GetName() == ConstantName)
		{
			OutConstant = FNiagaraVariable(Constant.GetType(), ConstantName);
			return true;
		}
	}
	ensureMsgf(false, TEXT("GeoNiagaraBuilderUtil: no input '%s' on '%s' — compile once after adding the function"),
			   *InputName.ToString(), *FunctionName.ToString());
	return false;
}

static UEdGraphPin* GetOverridePin(UNiagaraNodeFunctionCall& FunctionNode, FName InputName,
								   FNiagaraTypeDefinition const& InputType)
{
	FNiagaraParameterHandle const AliasedHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
		FNiagaraParameterHandle::CreateModuleParameterHandle(InputName), &FunctionNode);
	return &FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(FunctionNode, AliasedHandle,
																				  InputType, FGuid(), FGuid());
}

/** Niagara packs bools, ints and enums into the same 4 bytes a float occupies, so components convert uniformly. */
static int32 ToIntegerComponent(FNiagaraTypeDefinition const& Type, FString const& Text)
{
	if (Type == FNiagaraTypeDefinition::GetBoolDef())
	{
		return Text.ToBool() ? FNiagaraBool::True : FNiagaraBool::False;
	}

	UEnum const* Enum = Type.GetEnum();
	for (int32 EntryIndex = 0; Enum && EntryIndex < Enum->NumEnums() - 1; ++EntryIndex)
	{
		if (Enum->GetDisplayNameTextByIndex(EntryIndex).ToString() == Text)
		{
			return static_cast<int32>(Enum->GetValueByIndex(EntryIndex));
		}
	}
	return FCString::Atoi(*Text);
}

static bool WriteParameterValue(FNiagaraParameterStore& Store, FNiagaraVariable const& Variable, FString const& Text)
{
	int32 const ComponentCount = Variable.GetSizeInBytes() / sizeof(int32);
	TArray<FString> Components;
	Text.ParseIntoArray(Components, TEXT(","), true);
	if (!ensureMsgf(Components.Num() == ComponentCount, TEXT("GeoNiagaraBuilderUtil: '%s' takes %d components, got %d"),
					*Variable.GetName().ToString(), ComponentCount, Components.Num()))
	{
		return false;
	}

	FNiagaraTypeDefinition const& Type = Variable.GetType();
	bool const bIsIntegral =
		Type.IsEnum() || Type == FNiagaraTypeDefinition::GetIntDef() || Type == FNiagaraTypeDefinition::GetBoolDef();

	TArray<int32> ComponentData;
	for (FString const& Component : Components)
	{
		FString const Trimmed = Component.TrimStartAndEnd();
		if (bIsIntegral)
		{
			ComponentData.Add(ToIntegerComponent(Type, Trimmed));
			continue;
		}
		float const FloatComponent = FCString::Atof(*Trimmed);
		ComponentData.Add(*reinterpret_cast<int32 const*>(&FloatComponent));
	}
	Store.SetParameterData(reinterpret_cast<uint8 const*>(ComponentData.GetData()), Variable, true);
	return true;
}

FName UGeoNiagaraBuilderUtil::AddModule(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
										FString ModuleScriptPath, int32 TargetIndex)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	if (!Stage.IsValid())
	{
		return NAME_None;
	}

	UNiagaraScript* ModuleScript = LoadObject<UNiagaraScript>(nullptr, *ModuleScriptPath);
	UNiagaraNodeOutput* OutputNode = FNiagaraEditorUtilities::GetScriptOutputNode(*Stage.Script);
	if (!ensureMsgf(ModuleScript && OutputNode, TEXT("GeoNiagaraBuilderUtil: cannot add '%s' to '%s'"),
					*ModuleScriptPath, *EmitterName.ToString()))
	{
		return NAME_None;
	}

	UNiagaraNodeFunctionCall* ModuleNode =
		FNiagaraStackGraphUtilities::AddScriptModuleToStack(ModuleScript, *OutputNode, TargetIndex);
	if (!ensureMsgf(ModuleNode, TEXT("GeoNiagaraBuilderUtil: AddScriptModuleToStack failed for '%s'"),
					*ModuleScriptPath))
	{
		return NAME_None;
	}
	Stage.System->MarkPackageDirty();
	return FName(*ModuleNode->GetFunctionName());
}

bool UGeoNiagaraBuilderUtil::SetModuleEnabled(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
											  FName FunctionName, bool bEnabled)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	UNiagaraNodeFunctionCall* FunctionNode =
		Stage.IsValid() ? FindFunctionNode(*Stage.Script, Usage, FunctionName) : nullptr;
	if (!FunctionNode)
	{
		return false;
	}

	FNiagaraStackGraphUtilities::SetModuleIsEnabled(*FunctionNode, bEnabled);
	Stage.System->MarkPackageDirty();
	return true;
}

bool UGeoNiagaraBuilderUtil::SetStaticSwitch(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
											 FName FunctionName, FName SwitchName, FString EnumEntryDisplayName)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	UNiagaraNodeFunctionCall* FunctionNode =
		Stage.IsValid() ? FindFunctionNode(*Stage.Script, Usage, FunctionName) : nullptr;
	UEdGraphPin* SwitchPin = FunctionNode ? FindSwitchPin(*FunctionNode, SwitchName) : nullptr;
	UEnum const* SwitchEnum = SwitchPin ? Cast<UEnum>(SwitchPin->PinType.PinSubCategoryObject.Get()) : nullptr;
	if (!ensureMsgf(SwitchEnum, TEXT("GeoNiagaraBuilderUtil: '%s' input '%s' is not an enum switch"),
					*FunctionName.ToString(), *SwitchName.ToString()))
	{
		return false;
	}

	for (int32 EntryIndex = 0; EntryIndex < SwitchEnum->NumEnums() - 1; ++EntryIndex)
	{
		if (SwitchEnum->GetDisplayNameTextByIndex(EntryIndex).ToString() != EnumEntryDisplayName)
		{
			continue;
		}
		SwitchPin->DefaultValue = SwitchEnum->GetNameStringByIndex(EntryIndex);
		FunctionNode->MarkNodeRequiresSynchronization(TEXT("GeoNiagaraBuilderUtil set static switch"), true);
		Stage.System->MarkPackageDirty();
		return true;
	}
	ensureMsgf(false, TEXT("GeoNiagaraBuilderUtil: switch '%s' has no entry named '%s'"), *SwitchName.ToString(),
			   *EnumEntryDisplayName);
	return false;
}

bool UGeoNiagaraBuilderUtil::SetInputValue(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
										   FName FunctionName, FName InputName, FString Value)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	FNiagaraVariable Constant;
	if (!Stage.IsValid() || !FindInputConstant(Stage, Usage, FunctionName, InputName, Constant)
		|| !WriteParameterValue(Stage.Script->RapidIterationParameters, Constant, Value))
	{
		return false;
	}
	Stage.System->MarkPackageDirty();
	return true;
}

bool UGeoNiagaraBuilderUtil::SetInputLinkedParameter(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
													 FName FunctionName, FName InputName, FName LinkedParameterName)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	UNiagaraNodeFunctionCall* FunctionNode =
		Stage.IsValid() ? FindFunctionNode(*Stage.Script, Usage, FunctionName) : nullptr;
	FNiagaraVariable Constant;
	if (!FunctionNode || !FindInputConstant(Stage, Usage, FunctionName, InputName, Constant))
	{
		return false;
	}

	UEdGraphPin* OverridePin = GetOverridePin(*FunctionNode, InputName, Constant.GetType());
	FNiagaraVariableBase const LinkedParameter(Constant.GetType(), LinkedParameterName);
	FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(*OverridePin, LinkedParameter,
																		TSet<FNiagaraVariableBase>());
	Stage.System->MarkPackageDirty();
	return true;
}

FName UGeoNiagaraBuilderUtil::SetInputDynamicInput(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
												   FName FunctionName, FName InputName,
												   FString DynamicInputScriptPath)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	UNiagaraNodeFunctionCall* FunctionNode =
		Stage.IsValid() ? FindFunctionNode(*Stage.Script, Usage, FunctionName) : nullptr;
	UNiagaraScript* DynamicInputScript = LoadObject<UNiagaraScript>(nullptr, *DynamicInputScriptPath);
	FNiagaraVariable Constant;
	if (!ensureMsgf(FunctionNode && DynamicInputScript, TEXT("GeoNiagaraBuilderUtil: cannot nest '%s' under '%s.%s'"),
					*DynamicInputScriptPath, *FunctionName.ToString(), *InputName.ToString())
		|| !FindInputConstant(Stage, Usage, FunctionName, InputName, Constant))
	{
		return NAME_None;
	}

	UEdGraphPin* OverridePin = GetOverridePin(*FunctionNode, InputName, Constant.GetType());
	UNiagaraNodeFunctionCall* DynamicInputNode = nullptr;
	FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(*OverridePin, DynamicInputScript, DynamicInputNode);
	if (!ensureMsgf(DynamicInputNode, TEXT("GeoNiagaraBuilderUtil: SetDynamicInputForFunctionInput failed for '%s'"),
					*DynamicInputScriptPath))
	{
		return NAME_None;
	}
	Stage.System->MarkPackageDirty();
	return FName(*DynamicInputNode->GetFunctionName());
}

bool UGeoNiagaraBuilderUtil::SetUserParameter(FString SystemPath, FName ParameterName, FString Value)
{
	UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
	if (!System)
	{
		return false;
	}

	FNiagaraParameterStore& Store = System->GetExposedParameters();
	for (FNiagaraVariableWithOffset const& Parameter : Store.ReadParameterVariables())
	{
		if (Parameter.GetName() != ParameterName)
		{
			continue;
		}
		if (!WriteParameterValue(Store, FNiagaraVariable(Parameter.GetType(), Parameter.GetName()), Value))
		{
			return false;
		}
		System->MarkPackageDirty();
		return true;
	}
	ensureMsgf(false, TEXT("GeoNiagaraBuilderUtil: no exposed parameter '%s' on '%s'"), *ParameterName.ToString(),
			   *SystemPath);
	return false;
}

void UGeoNiagaraBuilderUtil::ListStack(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage)
{
	FGeoNiagaraStage const Stage = FindStage(SystemPath, EmitterName, Usage);
	if (!Stage.IsValid())
	{
		return;
	}

	TArray<UNiagaraNodeFunctionCall*> FunctionNodes;
	CollectFunctionNodes(*Stage.Script, Usage, FunctionNodes);
	for (UNiagaraNodeFunctionCall const* FunctionNode : FunctionNodes)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *FunctionNode->GetFunctionName());
		for (UEdGraphPin const* Pin : FunctionNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input && Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
			{
				UE_LOG(LogTemp, Display, TEXT("    switch %-38s = %s"), *Pin->PinName.ToString(), *Pin->DefaultValue);
			}
		}

		FString const InputSentinel = TEXT("GeoInputSentinel");
		FString ConstantPrefix =
			MakeConstantName(Stage, Usage, FName(*FunctionNode->GetFunctionName()), FName(*InputSentinel)).ToString();
		ConstantPrefix.LeftChopInline(InputSentinel.Len());
		for (FNiagaraVariableWithOffset const& Constant :
			 Stage.Script->RapidIterationParameters.ReadParameterVariables())
		{
			FString const ConstantName = Constant.GetName().ToString();
			if (ConstantName.StartsWith(ConstantPrefix))
			{
				UE_LOG(LogTemp, Display, TEXT("    input  %-38s %-22s [%d components]"),
					   *ConstantName.RightChop(ConstantPrefix.Len()), *Constant.GetType().GetName(),
					   Constant.GetSizeInBytes() / static_cast<int32>(sizeof(int32)));
			}
		}
	}
}

bool UGeoNiagaraBuilderUtil::CompileAndSave(FString SystemPath)
{
	UNiagaraSystem* System = LoadNiagaraSystem(SystemPath);
	if (!System)
	{
		return false;
	}

	System->RequestCompile(true);
	System->PollForCompilationComplete();
	return UEditorLoadingAndSavingUtils::SavePackages({ System->GetPackage() }, false);
}
