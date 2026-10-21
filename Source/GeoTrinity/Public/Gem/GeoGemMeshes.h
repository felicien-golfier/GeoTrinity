// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

enum class EGeoGemTier : uint8;
class UStaticMesh;

namespace GeoGem
{
	/** Shard shapes there are: SM_Shard_A, B and C. */
	constexpr int32 ShardVariantCount = 3;

	/** The static mesh /Game/Art/Meshes/Gem/AssetName, not loaded. */
	GEOTRINITY_API TSoftObjectPtr<UStaticMesh> GetMesh(FString const& AssetName);

	/** The solid of Tier: SM_Gem_<Tier>. */
	GEOTRINITY_API TSoftObjectPtr<UStaticMesh> GetTierMesh(EGeoGemTier Tier);

	/** The crystal fragment of Variant (0 .. ShardVariantCount - 1): SM_Shard_<A|B|C>. */
	GEOTRINITY_API TSoftObjectPtr<UStaticMesh> GetShardMesh(int32 Variant);
} // namespace GeoGem
