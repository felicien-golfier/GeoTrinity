// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gem/GeoGemTypes.h"
#include "Tool/GeoDifficulty.h"

#include "GeoGemCatalog.generated.h"

/**
 * Every gem type of the game, the Forge's prices, what kills drop and how class XP is earned. One asset, referenced by
 * UGameDataSettings::GemCatalog.
 */
UCLASS(BlueprintType)
class GEOTRINITY_API UGeoGemCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The catalog UGameDataSettings points at, kept resident. Null only when the setting is left empty. */
	static UGeoGemCatalog const* Get();

	/** The gem whose Id is GemId, or null for an id no gem carries (one removed from the catalog since it was saved). */
	FGeoGemInfo const* Find(FName GemId) const;

	/** The tier the gem whose Id is GemId is filed under; unset for an id no gem carries. */
	TOptional<EGeoGemTier> FindTier(FName GemId) const;

	/** Shards it costs to craft one gem of Tier. */
	int32 GetCraftCost(EGeoGemTier Tier) const;

	/** Shards breaking one gem of Tier down gives back. */
	int32 GetBreakDownShards(EGeoGemTier Tier) const;

	/**
	 * Gems one player gets from killing a BossType at Difficulty: its drop table's count scaled by DifficultyLootScale,
	 * each gem's tier rolled from the table's chances, the gem picked evenly among that tier's gems.
	 */
	TArray<FGeoGemStack> RollLoot(EGeoBossType BossType, EGeoDifficulty Difficulty, FRandomStream& Stream) const;

	/** Class XP for taking HealthTakenRatio of a BossType's health bar at Difficulty, whether it died or not. */
	int32 GetXp(EGeoBossType BossType, float HealthTakenRatio, EGeoDifficulty Difficulty) const;

	/** Every gem type, filed under its tier: a gem fits the sockets of that tier only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TMap<EGeoGemTier, FGeoGemList> GemsByTier = {{EGeoGemTier::Chip, FGeoGemList()},
												 {EGeoGemTier::Cut, FGeoGemList()},
												 {EGeoGemTier::Prism, FGeoGemList()},
												 {EGeoGemTier::Core, FGeoGemList()}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Forge")
	TMap<EGeoGemTier, int32> CraftCosts = {
		{EGeoGemTier::Chip, 5}, {EGeoGemTier::Cut, 25}, {EGeoGemTier::Prism, 125}, {EGeoGemTier::Core, 625}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Forge")
	TMap<EGeoGemTier, int32> BreakDownShards = {
		{EGeoGemTier::Chip, 1}, {EGeoGemTier::Cut, 5}, {EGeoGemTier::Prism, 25}, {EGeoGemTier::Core, 125}};

	/** What each kind of kill drops, at a difficulty scale of 1. Cores never drop from a boss: they are crafted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Loot")
	TMap<EGeoBossType, FGeoGemDropTable> DropTables;

	/** Multiplies a kill's drop counts. A failed attempt drops nothing at any difficulty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Loot")
	TMap<EGeoDifficulty, float> DifficultyLootScale = {
		{EGeoDifficulty::Safe, 0.5f}, {EGeoDifficulty::Reduced, 0.75f}, {EGeoDifficulty::Original, 1.f}};

	/** Class XP between two levels. The same for every level, so leveling is linear. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|ClassXp")
	int32 XpPerLevel = 1000;

	/**
	 * Class XP for taking a whole health bar of each kind of enemy, at a difficulty scale of 1. 400 at about five kills
	 * an hour is a level per half hour for a good player, so level 20 lands at 10 to 15 hours.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|ClassXp")
	TMap<EGeoBossType, float> XpPerHealthBar = {{EGeoBossType::Boss, 400.f}, {EGeoBossType::MiniBoss, 400.f}};

	/** Multiplies the class XP an attempt gives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|ClassXp")
	TMap<EGeoDifficulty, float> DifficultyXpScale = {
		{EGeoDifficulty::Safe, 0.5f}, {EGeoDifficulty::Reduced, 0.75f}, {EGeoDifficulty::Original, 1.f}};

private:
	/** Scales' entry for Difficulty; 0 with an ensure when it has none. */
	float GetDifficultyScale(TMap<EGeoDifficulty, float> const& Scales, EGeoDifficulty Difficulty) const;
};
