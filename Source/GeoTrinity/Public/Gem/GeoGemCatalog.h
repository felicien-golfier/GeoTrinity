// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gem/GeoGemTypes.h"
#include "Tool/GeoDifficulty.h"

#include "GeoGemCatalog.generated.h"

class UCurveFloat;

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

	/** The gem whose Id is GemId, or null for an id no gem carries (one removed from the catalog since it was saved).
	 */
	FGeoGemInfo const* Find(FName GemId) const;

	/** The tier the gem whose Id is GemId is filed under; unset for an id no gem carries. */
	TOptional<EGeoGemTier> FindTier(FName GemId) const;

	/** Shards it costs to craft one gem of Tier. */
	int32 GetCraftCost(EGeoGemTier Tier) const;

	/** Shards breaking one gem of Tier down gives back. */
	int32 GetBreakDownShards(EGeoGemTier Tier) const;

	/**
	 * Gems one player gets from DropTable at Difficulty: its count scaled by DifficultyLootScale, each gem's tier
	 * rolled from the table's chances, the gem picked evenly among that tier's gems.
	 */
	TArray<FGeoGemStack> RollLoot(FGeoGemDropTable const& DropTable, EGeoDifficulty Difficulty,
								  FRandomStream& Stream) const;

	/**
	 * What one player gets from killing a BossType with BossLoot at Difficulty: the roll of BossLoot's drop table (or
	 * the DropTables entry of BossType) plus its fixed gems and shards, which difficulty never scales. A fixed gem also
	 * rolled merges into one stack. Only Gems and Shards of the reward are set.
	 */
	FGeoGemReward MakeKillLoot(FGeoBossLoot const& BossLoot, EGeoBossType BossType, EGeoDifficulty Difficulty,
							   FRandomStream& Stream) const;

	/** Why BossLoot's fixed gems are wrong: a GemId no catalog gem carries, a Count under 1. Empty when valid. */
	TArray<FString> FindBossLootErrors(FGeoBossLoot const& BossLoot) const;

	/** Every catalog gem Id, Chip tier to Core: the dropdown of FGeoGemLootEntry::GemId. */
	UFUNCTION()
	static TArray<FName> GetGemIdOptions();

	/** Class XP for taking HealthTakenRatio of a BossType's health bar at Difficulty, whether it died or not. */
	int32 GetXp(EGeoBossType BossType, float HealthTakenRatio, EGeoDifficulty Difficulty) const;

	/**
	 * Share of an echoed gem's max per class that echo Level gives to Tier: EchoPercentPerLevel for each level up to
	 * Level whose turn in the EchoTierOrder cycle is Tier's. 0 for a tier outside the order.
	 */
	float GetEchoShare(EGeoGemTier Tier, int32 Level) const;

	/**
	 * How many slotted copies a Tier gem in the echo socket is worth at EchoLevel: the share GetEchoShare gives of the
	 * tier's whole socket count. The echo's value is the gem's MagnitudePerGem times this.
	 */
	float GetEchoCopies(EGeoGemTier Tier, int32 EchoLevel) const;

	/** Class XP echo level Level costs to reach from the one before: EchoXpCurve at Level, rounded. */
	int32 GetEchoXpForLevel(int32 Level) const;

	/** Every gem type, filed under its tier: a gem fits the sockets of that tier only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TMap<EGeoGemTier, FGeoGemList> GemsByTier = {{EGeoGemTier::Chip, FGeoGemList()},
												 {EGeoGemTier::Cut, FGeoGemList()},
												 {EGeoGemTier::Prism, FGeoGemList()},
												 {EGeoGemTier::Core, FGeoGemList()}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Forge")
	TMap<EGeoGemTier, int32> CraftCosts = {{EGeoGemTier::Chip, 5},
										   {EGeoGemTier::Cut, 25},
										   {EGeoGemTier::Prism, 125},
										   {EGeoGemTier::Core, 625}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Forge")
	TMap<EGeoGemTier, int32> BreakDownShards = {{EGeoGemTier::Chip, 1},
												{EGeoGemTier::Cut, 5},
												{EGeoGemTier::Prism, 25},
												{EGeoGemTier::Core, 125}};

	/** What each kind of kill drops, at a difficulty scale of 1. Cores never drop from a boss: they are crafted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Loot")
	TMap<EGeoBossType, FGeoGemDropTable> DropTables;

	/** Multiplies a kill's drop counts. A failed attempt drops nothing at any difficulty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Loot")
	TMap<EGeoDifficulty, float> DifficultyLootScale = {{EGeoDifficulty::Safe, 0.5f},
													   {EGeoDifficulty::Reduced, 0.75f},
													   {EGeoDifficulty::Original, 1.f}};

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
	TMap<EGeoDifficulty, float> DifficultyXpScale = {{EGeoDifficulty::Safe, 0.5f},
													 {EGeoDifficulty::Reduced, 0.75f},
													 {EGeoDifficulty::Original, 1.f}};

	/** Share of its tier's max per class each echo level adds to the one tier whose turn it is: 0.01 is 1%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Echo", meta = (ClampMin = 0, ClampMax = 1))
	float EchoPercentPerLevel = 0.01f;

	/** The tiers echo levels raise in turn: level 1 the first, level 2 the second, and round again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Echo")
	TArray<EGeoGemTier> EchoTierOrder = {EGeoGemTier::Chip, EGeoGemTier::Cut, EGeoGemTier::Prism};

	/** Highest echo level; class XP earned past it is lost. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Echo", meta = (ClampMin = 1))
	int32 EchoMaxLevel = 100;

	/** X = the echo level being reached (2 to EchoMaxLevel), Y = the class XP it costs. Required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Echo")
	TObjectPtr<UCurveFloat> EchoXpCurve;

private:
	/** Adds Count copies of GemId to Stacks, onto its stack when it has one. */
	static void AddToStacks(TArray<FGeoGemStack>& Stacks, FName GemId, int32 Count);

	/** Scales' entry for Difficulty; 0 with an ensure when it has none. */
	float GetDifficultyScale(TMap<EGeoDifficulty, float> const& Scales, EGeoDifficulty Difficulty) const;
};
