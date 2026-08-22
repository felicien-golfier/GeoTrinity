// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "NiagaraCommon.h"

#include "GeoNiagaraBuilderUtil.generated.h"

/**
 * Editor utility object for mutating UNiagaraSystem emitter stacks from Python or Blueprint automation.
 * Niagara's stack view models only exist while the asset editor is open, so these expose the graph
 * operations Python cannot reach; compose everything else from Python on top of them.
 *
 * `FunctionName` addresses any function call node in a stage — a module *or* a dynamic input — by the name
 * it carries in its rapid-iteration constants (`Constants.<Emitter>.<FunctionName>.<InputName>`). Nesting
 * therefore needs no special API: SetInputDynamicInput returns the new node's function name, and that name
 * is a valid `FunctionName` for any call below, to any depth.
 *
 * `Value` is a comma separated component list matching the parameter's component count: floats for numeric
 * types ("40", "0,0,-980"), an integer or an enum entry's display name for enums ("Cylinder"), and anything
 * FString::ToBool accepts for bools ("true"). Renderer properties are plain UPROPERTYs — set those straight
 * from Python instead.
 *
 * Every mutation only dirties the asset; call CompileAndSave once per batch. Only static switches are pins on a
 * function call node — a value input is addressable through the rapid-iteration constant the compiler emits for
 * it, so a freshly added function needs one CompileAndSave before its values can be set.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoNiagaraBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Inserts a module script into one emitter script stage.
	 *
	 * @param TargetIndex  Position in the stage's module list; pass -1 to append.
	 * @return The function name the module was registered under, or NAME_None on failure.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static FName AddModule(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
						   FString ModuleScriptPath, int32 TargetIndex = -1);

	/** Enables or disables one module in a stage's stack. Returns false when the system, emitter, or module could not be found. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool SetModuleEnabled(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
								 FName FunctionName, bool bEnabled);

	/** Selects a compile-time switch entry by display name (e.g. SwitchName "Shape Primitive", entry "Cylinder"). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool SetStaticSwitch(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
								FName FunctionName, FName SwitchName, FString EnumEntryDisplayName);

	/** Writes an input's constant value. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool SetInputValue(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
							  FName FunctionName, FName InputName, FString Value);

	/** Binds an input to another parameter in scope (e.g. "Particles.Position", "User.EffectColor"). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool SetInputLinkedParameter(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
										FName FunctionName, FName InputName, FName LinkedParameterName);

	/**
	 * Feeds an input from a dynamic input script, nesting it under that input.
	 *
	 * @return The new node's function name, used to address its own inputs; NAME_None on failure.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static FName SetInputDynamicInput(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage,
									  FName FunctionName, FName InputName, FString DynamicInputScriptPath);

	/** Writes a value onto one of the system's exposed User parameters. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool SetUserParameter(FString SystemPath, FName ParameterName, FString Value);

	/** Logs every function call node of one stage to LogTemp with its inputs, their types and component counts. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ListStack(FString SystemPath, FName EmitterName, ENiagaraScriptUsage Usage);

	/** Compiles and saves the Niagara system at SystemPath. Call once after batching mutations. Returns false if the asset was not found or compilation failed. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static bool CompileAndSave(FString SystemPath);
};
