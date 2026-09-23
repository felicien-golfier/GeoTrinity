// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoMaterialBuilderUtil.generated.h"

class UMaterialFunctionInterface;
class UMaterialInstanceConstant;

/**
 * Generic material-authoring primitives for Python/Blueprint automation.
 *
 * These exist because a material instance's own layer stack is unreachable from Python: it lives in the instance's
 * static parameters, which no property exposes, and UMaterialInstance::SetMaterialLayers is C++ only.
 *
 * Keep this class free of per-asset functions: it operates on any asset from caller-supplied arguments.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoMaterialBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Generic: puts Layer at LayerIndex of Instance's layer stack, unlinked from the parent's so it stays whatever the
	 * parent does, and recompiles the instance. Every other layer keeps following the parent. Saves the asset.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void SetInstanceLayer(UMaterialInstanceConstant* Instance, int32 LayerIndex,
								 UMaterialFunctionInterface* Layer);
};
