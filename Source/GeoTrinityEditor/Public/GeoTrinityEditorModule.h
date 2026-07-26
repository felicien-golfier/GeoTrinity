// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Editor-only module hosting the Python/Blueprint automation utilities (UEditorUtilityObject subclasses) that
 * mutate StateTree and Widget assets, plus the project's details customizations. Lives apart from the runtime
 * GeoTrinity module so these editor-only types are never compiled into packaged Game/Shipping builds.
 */
class FGeoTrinityEditorModule : public IModuleInterface
{
public:
	/** Registers the project's property type customizations. */
	virtual void StartupModule() override;
	/** Unregisters them again. */
	virtual void ShutdownModule() override;
};
