// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Tool/GeoColor.h"

#include "GeoGemTypes.generated.h"

/** A gem's whole identity: it only fits a socket of its own tier. */
UENUM(BlueprintType)
enum class EGeoGemTier : uint8
{
	Chip,
	Cut,
	Prism,
	Core
};

/** How a stat gem's summed magnitude lands on its attribute. */
UENUM(BlueprintType)
enum class EGeoGemOperation : uint8
{
	/** Added to the attribute's base, for attributes that start at 0 (DamageReduction) or are already a multiplier. */
	Add,
	/** Scales the attribute by 1 + the summed magnitude, for flat values (MaxHealth, MaxAmmo). */
	Percent
};

/** One gem type, as listed in UGeoGemCatalog under its tier. */
USTRUCT(BlueprintType)
struct FGeoGemInfo
{
	GENERATED_BODY()

	/** Key the save file and the loadouts store; renaming it orphans every copy players own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FText DisplayName;

	/** What the gem does, as the menus say it: the stat it changes ("Dash cooldown"), or a Core's whole rule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FText Effect;

	/** Its stat family, which the menus draw it in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FGeoColorParam Color;

	/** Stat the gem raises; left empty by a gem whose effect is not a stat. Must be one UGeoGemStatsEffect lists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	EGeoGemOperation Operation = EGeoGemOperation::Add;

	/** Tag the player holds while the gem is slotted, which the code of a Core's rule checks; empty on a stat gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FGameplayTag GrantedTag;

	/** What one slotted copy adds, as a fraction: 0.002 is +0.2%. Slotted copies sum. Also what the menus show for a
	 * gem whose effect is not a stat yet; 0 on a Core, whose Effect is its rule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	float MagnitudePerGem = 0.f;

	/** What Count slotted copies add as the menus show it, "+0.67%"; empty for a gem with no magnitude (a Core). */
	GEOTRINITY_API FText GetMagnitudeText(int32 Count = 1) const;

	/** The effect of Count slotted copies as the menus show it: "Damage +0.67%", or a Core's rule alone. */
	GEOTRINITY_API FText GetEffectText(int32 Count = 1) const;
};

/** The gem types of one tier. */
USTRUCT(BlueprintType)
struct FGeoGemList
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TArray<FGeoGemInfo> Gems;

	FGeoGemInfo const* Find(FName GemId) const
	{
		return Gems.FindByPredicate(
			[GemId](FGeoGemInfo const& Gem)
			{
				return Gem.Id == GemId;
			});
	}
};

/** What an arena's enemy is: picks the drop table a kill rolls and what its health bar is worth in class XP. */
UENUM(BlueprintType)
enum class EGeoBossType : uint8
{
	Boss,
	MiniBoss
};

/**
 * What one kill drops before the difficulty scale: MinCount to MaxCount gems, each rolled from the highest tier down. A
 * gem is a Core at the Core chance, else a Prism at the Prism chance, else a Cut at the Cut chance, else a Chip.
 */
USTRUCT(BlueprintType)
struct FGeoGemDropTable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = 0))
	int32 MinCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = 0))
	int32 MaxCount = 0;

	/** Chance of each gem being of a tier above Chip, once every higher tier missed: 0.03 is 3%. A tier left out never
	 *  drops; a Chip entry is ignored, Chip being what is left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = 0, ClampMax = 1))
	TMap<EGeoGemTier, float> TierChances;
};

/** Count copies of the gem Id. */
USTRUCT(BlueprintType)
struct FGeoGemStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem")
	int32 Count = 0;
};

/** What one player earns from a finished attempt: class XP for the class they played, and the gems of a kill. */
USTRUCT(BlueprintType)
struct FGeoGemReward
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem")
	EPlayerClass PlayerClass = EPlayerClass::None;

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem")
	int32 Xp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem")
	TArray<FGeoGemStack> Gems;
};

/** One socket of a class's board: the tier it takes and the class level that opens it. */
struct FGeoGemSocket
{
	EGeoGemTier Tier;
	int32 UnlockLevel;
};

/** The gems slotted in one class's board, one entry per GeoGem::GetSockets() index; NAME_None is an empty socket. */
USTRUCT(BlueprintType)
struct FGeoGemLoadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GeoGem")
	TArray<FName> Sockets;
};

namespace GeoGem
{
	constexpr int32 MaxClassLevel = 20;

	/** Builds one class keeps at most. */
	constexpr int32 MaxBuilds = 10;

	/** Characters a build's name keeps at most. */
	constexpr int32 MaxBuildNameLength = 14;

	/**
	 * Every socket of a class's board: three clusters, each opened over seven class levels in the same seven steps
	 * (cluster I at levels 1-7, II at 8-14, III at 15-20, its last two steps both at 20), the Core always in the last
	 * step. The order is the loadout's
	 * index order, so it is part of the save format: append, never reorder.
	 */
	GEOTRINITY_API TArray<FGeoGemSocket> const& GetSockets();
} // namespace GeoGem
