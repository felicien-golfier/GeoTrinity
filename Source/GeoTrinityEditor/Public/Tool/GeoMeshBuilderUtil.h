// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"

#include "GeoMeshBuilderUtil.generated.h"

class UStaticMesh;

/**
 * Generic static-mesh primitives for Python automation (AI/Python/Mesh/static_mesh_asset.py).
 *
 * These exist because UStaticMesh::bDoFastBuild has no editor or Blueprint access: Python cannot clear it.
 *
 * Keep this class free of per-asset functions: it operates on any asset from caller-supplied arguments.
 */
UCLASS()
class GEOTRINITYEDITOR_API UGeoMeshBuilderUtil : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/**
	 * Generic: clears Mesh's fast-build flag, set for good by any fast BuildFromStaticMeshDescriptions, and rebuilds
	 * its render data the editor way under a new cache key. A flagged mesh rebuilt on a derived-data cache miss (any
	 * edit, a cold cache) takes the engine's fast path, which leaves its bounds unset and stores them so in the cache
	 * under a key the flag is no part of: garbage / NaN bounds. Does not save; no-op on an unflagged mesh.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
	static void ClearFastBuild(UStaticMesh* Mesh);
};
