// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemMeshes.h"

#include "Engine/StaticMesh.h"
#include "Gem/GeoGemTypes.h"

TSoftObjectPtr<UStaticMesh> GeoGem::GetMesh(FString const& AssetName)
{
	return TSoftObjectPtr<UStaticMesh>(
		FSoftObjectPath(FString::Printf(TEXT("/Game/Art/Meshes/Gem/%s.%s"), *AssetName, *AssetName)));
}

TSoftObjectPtr<UStaticMesh> GeoGem::GetTierMesh(EGeoGemTier const Tier)
{
	return GetMesh(
		FString::Printf(TEXT("SM_Gem_%s"), *StaticEnum<EGeoGemTier>()->GetNameStringByValue(static_cast<int64>(Tier))));
}

TSoftObjectPtr<UStaticMesh> GeoGem::GetShardMesh(int32 const Variant)
{
	return GetMesh(FString::Printf(TEXT("SM_Shard_%c"), static_cast<TCHAR>(TEXT('A') + Variant)));
}
