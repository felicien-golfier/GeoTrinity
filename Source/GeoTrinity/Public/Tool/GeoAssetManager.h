// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"

#include "GeoAssetManager.generated.h"

/**
 * Custom asset manager for GeoTrinity. Hooks into StartInitialLoading to register
 * native gameplay tags via FGeoGameplayTags before any assets are loaded.
 * Set as the AssetManagerClassName in DefaultEngine.ini.
 */
UCLASS()
class GEOTRINITY_API UGeoAssetManager : public UAssetManager
{
	GENERATED_BODY()
public:
	/** Returns the global UGeoAssetManager instance. Asserts if the asset manager is not of this type. */
	static UGeoAssetManager& Get();

protected:
	/** Calls the parent load, then initializes native gameplay tags via FGeoGameplayTags. */
	virtual void StartInitialLoading() override;
};
