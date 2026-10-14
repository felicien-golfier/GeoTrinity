// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemCatalog.h"

#include "Algo/Count.h"
#include "Curves/CurveFloat.h"
#include "Settings/GameDataSettings.h"

UGeoGemCatalog const* UGeoGemCatalog::Get()
{
	UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
	UGeoGemCatalog const* Catalog = UGameDataSettings::GetLoadedDataAsset(GameDataSettings->GemCatalog);
	ensureMsgf(Catalog, TEXT("%hs: Game Data Settings has no GemCatalog"), __FUNCTION__);
	return Catalog;
}

FGeoGemInfo const* UGeoGemCatalog::Find(FName const GemId) const
{
	for (TPair<EGeoGemTier, FGeoGemList> const& Tier : GemsByTier)
	{
		if (FGeoGemInfo const* Gem = Tier.Value.Find(GemId))
		{
			return Gem;
		}
	}

	return nullptr;
}

TOptional<EGeoGemTier> UGeoGemCatalog::FindTier(FName const GemId) const
{
	for (TPair<EGeoGemTier, FGeoGemList> const& Tier : GemsByTier)
	{
		if (Tier.Value.Find(GemId))
		{
			return Tier.Key;
		}
	}

	return {};
}

int32 UGeoGemCatalog::GetCraftCost(EGeoGemTier const Tier) const
{
	int32 const* Cost = CraftCosts.Find(Tier);
	ensureMsgf(Cost, TEXT("%hs: %s has no craft cost for tier %d"), __FUNCTION__, *GetName(), static_cast<int32>(Tier));
	return Cost ? *Cost : 0;
}

int32 UGeoGemCatalog::GetBreakDownShards(EGeoGemTier const Tier) const
{
	int32 const* Shards = BreakDownShards.Find(Tier);
	ensureMsgf(Shards, TEXT("%hs: %s has no break-down value for tier %d"), __FUNCTION__, *GetName(), static_cast<int32>(Tier));
	return Shards ? *Shards : 0;
}

TArray<FGeoGemStack> UGeoGemCatalog::RollLoot(EGeoBossType const BossType, EGeoDifficulty const Difficulty,
											   FRandomStream& Stream) const
{
	FGeoGemDropTable const* DropTable = DropTables.Find(BossType);
	if (!ensureMsgf(DropTable, TEXT("%hs: %s has no drop table for boss type %d"), __FUNCTION__, *GetName(),
					static_cast<int32>(BossType)))
	{
		return {};
	}

	float const ScaledCount = Stream.RandRange(DropTable->MinCount, DropTable->MaxCount)
							* GetDifficultyScale(DifficultyLootScale, Difficulty);
	int32 const Count = FMath::FloorToInt32(ScaledCount) + (Stream.FRand() < FMath::Frac(ScaledCount) ? 1 : 0);
	TArray<FGeoGemStack> Loot;
	for (int32 i = 0; i < Count; ++i)
	{
		EGeoGemTier Tier = EGeoGemTier::Chip;
		for (EGeoGemTier const HigherTier : {EGeoGemTier::Core, EGeoGemTier::Prism, EGeoGemTier::Cut})
		{
			if (Tier == EGeoGemTier::Chip && Stream.FRand() < DropTable->TierChances.FindRef(HigherTier))
			{
				Tier = HigherTier;
			}
		}

		FGeoGemList const* TierGems = GemsByTier.Find(Tier);
		if (ensureMsgf(TierGems && !TierGems->Gems.IsEmpty(), TEXT("%hs: %s drops tier %d but lists no gem of it"),
					   __FUNCTION__, *GetName(), static_cast<int32>(Tier)))
		{
			FName const GemId = TierGems->Gems[Stream.RandHelper(TierGems->Gems.Num())].Id;
			FGeoGemStack* Stack = Loot.FindByPredicate(
				[GemId](FGeoGemStack const& Existing)
				{
					return Existing.Id == GemId;
				});
			if (Stack)
			{
				++Stack->Count;
			}
			else
			{
				Loot.Add({GemId, 1});
			}
		}
	}

	return Loot;
}

int32 UGeoGemCatalog::GetXp(EGeoBossType const BossType, float const HealthTakenRatio,
							EGeoDifficulty const Difficulty) const
{
	float const* XpPerBar = XpPerHealthBar.Find(BossType);
	if (!ensureMsgf(XpPerBar, TEXT("%hs: %s has no XP per health bar for boss type %d"), __FUNCTION__, *GetName(),
					static_cast<int32>(BossType)))
	{
		return 0;
	}

	return FMath::RoundToInt32(FMath::Clamp(HealthTakenRatio, 0.f, 1.f) * *XpPerBar
							   * GetDifficultyScale(DifficultyXpScale, Difficulty));
}

float UGeoGemCatalog::GetEchoShare(EGeoGemTier const Tier, int32 const Level) const
{
	int32 const Place = EchoTierOrder.Find(Tier);
	int32 const CycleLength = EchoTierOrder.Num();
	int32 const LevelCount = Place == INDEX_NONE || Level <= Place ? 0 : (Level - Place - 1) / CycleLength + 1;
	return EchoPercentPerLevel * LevelCount;
}

float UGeoGemCatalog::GetEchoCopies(EGeoGemTier const Tier, int32 const EchoLevel) const
{
	int32 const TierSocketCount = Algo::CountIf(GeoGem::GetSockets(),
												[Tier](FGeoGemSocket const& Socket)
												{
													return Socket.Tier == Tier;
												});
	return TierSocketCount * GetEchoShare(Tier, EchoLevel);
}

int32 UGeoGemCatalog::GetEchoXpForLevel(int32 const Level) const
{
	if (!ensureMsgf(EchoXpCurve, TEXT("%hs: %s has no EchoXpCurve"), __FUNCTION__, *GetName()))
	{
		return XpPerLevel;
	}

	return FMath::RoundToInt32(EchoXpCurve->GetFloatValue(Level));
}

float UGeoGemCatalog::GetDifficultyScale(TMap<EGeoDifficulty, float> const& Scales,
										 EGeoDifficulty const Difficulty) const
{
	float const* Scale = Scales.Find(Difficulty);
	ensureMsgf(Scale, TEXT("%hs: %s has no scale for difficulty %d"), __FUNCTION__, *GetName(),
			   static_cast<int32>(Difficulty));
	return Scale ? *Scale : 0.f;
}
