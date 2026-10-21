// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoMeshBuilderUtil.h"

#include "Engine/StaticMesh.h"

void UGeoMeshBuilderUtil::ClearFastBuild(UStaticMesh* Mesh)
{
	if (!ensureMsgf(Mesh, TEXT("ClearFastBuild needs a Mesh")))
	{
		return;
	}

	if (Mesh->bDoFastBuild)
	{
		Mesh->Modify();
		Mesh->bDoFastBuild = false;
		// A fresh mesh description id = a fresh cache key: the entries built on the fast path stay unread.
		for (int32 LodIndex = 0; LodIndex < Mesh->GetNumSourceModels(); ++LodIndex)
		{
			if (Mesh->GetMeshDescription(LodIndex))
			{
				Mesh->CommitMeshDescription(LodIndex);
			}
		}

		Mesh->Build(true);
	}
}
