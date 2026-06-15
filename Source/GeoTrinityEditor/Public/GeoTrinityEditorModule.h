// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Editor-only module hosting the Python/Blueprint automation utilities (UEditorUtilityObject subclasses) that
 * mutate StateTree and Widget assets. Lives apart from the runtime GeoTrinity module so these editor-only types
 * are never compiled into packaged Game/Shipping builds.
 */
class FGeoTrinityEditorModule : public IModuleInterface
{
};
