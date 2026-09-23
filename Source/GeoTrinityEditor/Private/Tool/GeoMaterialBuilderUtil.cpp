// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoMaterialBuilderUtil.h"

#include "FileHelpers.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialLayersFunctions.h"

void UGeoMaterialBuilderUtil::SetInstanceLayer(UMaterialInstanceConstant* Instance, int32 LayerIndex,
											   UMaterialFunctionInterface* Layer)
{
	if (!ensureMsgf(Instance && Layer, TEXT("SetInstanceLayer needs both an Instance and a Layer")))
	{
		return;
	}

	FMaterialLayersFunctions Stack;
	if (!ensureMsgf(Instance->GetMaterialLayers(Stack) && Stack.Layers.IsValidIndex(LayerIndex),
					TEXT("%s has no layer %d"), *Instance->GetName(), LayerIndex))
	{
		return;
	}

	Instance->Modify();
	Stack.Layers[LayerIndex] = Layer;
	Stack.UnlinkLayerFromParent(LayerIndex);
	{
		// The context recompiles the instance when it goes out of scope.
		FMaterialInstanceParameterUpdateContext UpdateContext(Instance);
		UpdateContext.SetMaterialLayers(Stack);
	}
	UEditorLoadingAndSavingUtils::SavePackages({Instance->GetPackage()}, false);
}
