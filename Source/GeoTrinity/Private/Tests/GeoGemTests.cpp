// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Algo/Count.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoGemTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

static FGeoGemInfo MakeGem(FName Id, FGameplayAttribute Attribute = FGameplayAttribute(),
						   EGeoGemOperation Operation = EGeoGemOperation::Add, float MagnitudePerGem = 0.f)
{
	FGeoGemInfo Gem;
	Gem.Id = Id;
	Gem.Attribute = Attribute;
	Gem.Operation = Operation;
	Gem.MagnitudePerGem = MagnitudePerGem;
	return Gem;
}

/** Three Chips, a Cut, a Prism and two Cores, with the doc's drop tables, free of any asset. */
static UGeoGemCatalog* MakeTestCatalog()
{
	UGeoGemCatalog* Catalog = NewObject<UGeoGemCatalog>();
	Catalog->GemsByTier[EGeoGemTier::Chip].Gems = {
		MakeGem("Power", UCharacterAttributeSet::GetDamageMultiplierAttribute(), EGeoGemOperation::Add, 0.002f),
		MakeGem("Guard", UCharacterAttributeSet::GetDamageReductionAttribute(), EGeoGemOperation::Add, 0.002f),
		MakeGem("Vigor", UCharacterAttributeSet::GetMaxHealthAttribute(), EGeoGemOperation::Percent, 0.0035f),
	};
	Catalog->GemsByTier[EGeoGemTier::Cut].Gems = {
		MakeGem("Magazine", UCharacterAttributeSet::GetMaxAmmoAttribute(), EGeoGemOperation::Percent, 0.012f)};
	Catalog->GemsByTier[EGeoGemTier::Prism].Gems = {MakeGem("Anchor")};
	Catalog->GemsByTier[EGeoGemTier::Core].Gems = {MakeGem("Surplus"), MakeGem("SecondWind")};

	FGeoGemDropTable& BossDrops = Catalog->DropTables.Add(EGeoBossType::Boss);
	BossDrops.MinCount = 38;
	BossDrops.MaxCount = 50;
	BossDrops.TierChances = {{EGeoGemTier::Prism, 0.03f}, {EGeoGemTier::Cut, 0.16f}};
	FGeoGemDropTable& MiniBossDrops = Catalog->DropTables.Add(EGeoBossType::MiniBoss);
	MiniBossDrops.MinCount = 38;
	MiniBossDrops.MaxCount = 50;
	MiniBossDrops.TierChances = {{EGeoGemTier::Cut, 0.16f}};
	return Catalog;
}

/** Index of the first socket of Tier that opens at exactly UnlockLevel. */
static int32 FindSocket(EGeoGemTier Tier, int32 UnlockLevel)
{
	return GeoGem::GetSockets().IndexOfByPredicate(
		[Tier, UnlockLevel](FGeoGemSocket const& Socket)
		{
			return Socket.Tier == Tier && Socket.UnlockLevel == UnlockLevel;
		});
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemSocketLayoutTest, "GeoTrinity.Gems.SocketLayout", GeoGemTestFlags)

bool FGeoGemSocketLayoutTest::RunTest(FString const& /*Parameters*/)
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	auto CountTier = [&Sockets](EGeoGemTier Tier)
	{
		return static_cast<int32>(Algo::CountIf(Sockets,
							 [Tier](FGeoGemSocket const& Socket)
							 {
								 return Socket.Tier == Tier;
							 }));
	};
	auto CountOpenAt = [&Sockets](int32 Level)
	{
		return static_cast<int32>(Algo::CountIf(Sockets,
							 [Level](FGeoGemSocket const& Socket)
							 {
								 return Socket.UnlockLevel <= Level;
							 }));
	};

	TestEqual(TEXT("Sockets per class"), Sockets.Num(), 57);
	TestEqual(TEXT("Chips"), CountTier(EGeoGemTier::Chip), 30);
	TestEqual(TEXT("Cuts"), CountTier(EGeoGemTier::Cut), 15);
	TestEqual(TEXT("Prisms"), CountTier(EGeoGemTier::Prism), 9);
	TestEqual(TEXT("Cores"), CountTier(EGeoGemTier::Core), 3);
	TestEqual(TEXT("Open at level 1: 2 Chips"), CountOpenAt(1), 2);
	TestEqual(TEXT("Open at level 7: a whole cluster"), CountOpenAt(7), 19);
	TestEqual(TEXT("Open at level 14: two clusters"), CountOpenAt(14), 38);
	TestEqual(TEXT("Open at level 19"), CountOpenAt(19), 51);
	TestEqual(TEXT("Open at level 20: everything"), CountOpenAt(20), 57);
	TestTrue(TEXT("Each cluster's Core opens last"), FindSocket(EGeoGemTier::Core, 7) != INDEX_NONE
		&& FindSocket(EGeoGemTier::Core, 14) != INDEX_NONE && FindSocket(EGeoGemTier::Core, 20) != INDEX_NONE);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemEquipRulesTest, "GeoTrinity.Gems.EquipRules", GeoGemTestFlags)

bool FGeoGemEquipRulesTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	int32 const LevelOneChip = FindSocket(EGeoGemTier::Chip, 1);
	int32 const LevelTwoChip = FindSocket(EGeoGemTier::Chip, 2);

	TestFalse(TEXT("No copy owned"), Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Power"));

	Profile->AddGems("Power", 3);
	TestFalse(TEXT("Socket locked above the class level"),
			  Profile->Equip(Catalog, EPlayerClass::Triangle, LevelTwoChip, "Power"));
	TestFalse(TEXT("Unknown gem"), Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Nothing"));

	TestEqual(TEXT("Fill at level 1 takes both open Chip sockets"),
			  Profile->FillEmptySockets(Catalog, EPlayerClass::Triangle, "Power"), 2);
	TestEqual(TEXT("Slotted copies are taken from the class's free count"),
			  Profile->GetFreeCount("Power", EPlayerClass::Triangle), 1);
	TestEqual(TEXT("Another class slots the same copies"),
			  Profile->FillEmptySockets(Catalog, EPlayerClass::Circle, "Power"), 2);
	TestEqual(TEXT("Its free count is its own"), Profile->GetFreeCount("Power", EPlayerClass::Circle), 1);
	TestEqual(TEXT("A copy slotted in two classes is reserved once"), Profile->GetBreakableCount("Power"), 1);
	TestEqual(TEXT("Owned counts every copy"), Profile->GetOwnedCount("Power"), 3);

	Profile->AddGems("Guard", 1);
	TestTrue(TEXT("Replacing a slotted gem"), Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Guard"));
	TestEqual(TEXT("The replaced copy goes back to free"), Profile->GetFreeCount("Power", EPlayerClass::Triangle), 2);
	TestEqual(TEXT("Circle still holds two"), Profile->GetBreakableCount("Power"), 1);

	Profile->Unequip(EPlayerClass::Circle, LevelOneChip);
	TestEqual(TEXT("Unequip frees the copy"), Profile->GetFreeCount("Power", EPlayerClass::Circle), 2);
	TestEqual(TEXT("No loadout holds two any more"), Profile->GetBreakableCount("Power"), 2);

	Profile->SetClassLevel(EPlayerClass::Triangle, 99);
	TestEqual(TEXT("Level clamps to the max"), Profile->GetClassLevel(EPlayerClass::Triangle), GeoGem::MaxClassLevel);
	TestFalse(TEXT("Chip in a Core socket"),
			  Profile->Equip(Catalog, EPlayerClass::Triangle, FindSocket(EGeoGemTier::Core, 7), "Power"));
	TestEqual(TEXT("At level 20 the remaining copies fill"),
			  Profile->FillEmptySockets(Catalog, EPlayerClass::Triangle, "Power"), 2);

	Profile->AddGems("Surplus", 2);
	TestEqual(TEXT("A Core fills one socket at most"),
			  Profile->FillEmptySockets(Catalog, EPlayerClass::Triangle, "Surplus"), 1);
	TestFalse(TEXT("A loadout holds each Core once"),
			  Profile->Equip(Catalog, EPlayerClass::Triangle, FindSocket(EGeoGemTier::Core, 14), "Surplus"));
	TestFalse(TEXT("A Core socket stays locked until its cluster's last level"),
			  Profile->Equip(Catalog, EPlayerClass::Circle, FindSocket(EGeoGemTier::Core, 7), "Surplus"));
	Profile->SetClassLevel(EPlayerClass::Circle, 7);
	TestTrue(TEXT("Another class shares the Core copy"),
			 Profile->Equip(Catalog, EPlayerClass::Circle, FindSocket(EGeoGemTier::Core, 7), "Surplus"));
	TestEqual(TEXT("The spare Core stays breakable"), Profile->GetBreakableCount("Surplus"), 1);

	FGeoGemLoadout const Loadout = Profile->GetLoadout(EPlayerClass::Triangle);
	TestEqual(TEXT("A loadout covers every socket"), Loadout.Sockets.Num(), GeoGem::GetSockets().Num());
	TestEqual(TEXT("Power slotted on Triangle"), static_cast<int32>(Algo::Count(Loadout.Sockets, FName("Power"))), 3);
	TestEqual(TEXT("Empty loadout of a class never touched"),
			  static_cast<int32>(Algo::Count(Profile->GetLoadout(EPlayerClass::Square).Sockets, NAME_None)),
			  GeoGem::GetSockets().Num());
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemForgeTest, "GeoTrinity.Gems.Forge", GeoGemTestFlags)

bool FGeoGemForgeTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();

	Profile->AddGems("Power", 4);
	Profile->FillEmptySockets(Catalog, EPlayerClass::Triangle, "Power");
	TestEqual(TEXT("Breaking down never touches slotted copies"), Profile->BreakDown(Catalog, "Power", 10), 2);
	TestEqual(TEXT("Slotted copies stay owned"), Profile->GetOwnedCount("Power"), 2);
	Profile->FillEmptySockets(Catalog, EPlayerClass::Circle, "Power");
	TestEqual(TEXT("Copies slotted in two classes are never broken down"), Profile->BreakDown(Catalog, "Power", 10), 0);
	TestEqual(TEXT("Shards from two Chips"), Profile->GetShards(), 2);

	Profile->AddGems("Guard", 10);
	Profile->AddGems("Vigor", 3);
	Profile->AddGems("Magazine", 2);
	TestEqual(TEXT("Breaking a tier down takes every free gem of it"), Profile->BreakDownTier(Catalog, EGeoGemTier::Chip),
			  13);
	TestEqual(TEXT("Other tiers untouched"), Profile->GetOwnedCount("Magazine"), 2);
	TestEqual(TEXT("Broken stacks are gone"), Profile->GetOwnedCount("Guard"), 0);
	TestEqual(TEXT("A Cut gives 5 shards"), Profile->BreakDown(Catalog, "Magazine", 1), 5);
	TestEqual(TEXT("Shards add up"), Profile->GetShards(), 20);

	TestFalse(TEXT("Craft refused when shards fall short"), Profile->Craft(Catalog, "Magazine", 1));
	TestTrue(TEXT("Craft four Chips at 5 shards"), Profile->Craft(Catalog, "Guard", 4));
	TestEqual(TEXT("Crafted copies land in the stack"), Profile->GetOwnedCount("Guard"), 4);
	TestEqual(TEXT("Craft spends the shards"), Profile->GetShards(), 0);
	TestFalse(TEXT("Zero quantity is refused"), Profile->Craft(Catalog, "Guard", 0));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemCatalogAssetTest, "GeoTrinity.Gems.CatalogAsset", GeoGemTestFlags)

bool FGeoGemCatalogAssetTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!TestNotNull(TEXT("Game Data Settings points at a gem catalog"), Catalog))
	{
		return false;
	}

	TSet<FName> Ids;
	for (TPair<EGeoGemTier, FGeoGemList> const& Tier : Catalog->GemsByTier)
	{
		for (FGeoGemInfo const& Gem : Tier.Value.Gems)
		{
			bool bDuplicate = false;
			Ids.Add(Gem.Id, &bDuplicate);
			TestFalse(FString::Printf(TEXT("%s: unique id"), *Gem.Id.ToString()), bDuplicate || Gem.Id.IsNone());
			TestTrue(FString::Printf(TEXT("%s: stat listed by UGeoGemStatsEffect"), *Gem.Id.ToString()),
					 UGeoGemStatsEffect::Supports(Gem));
		}
	}
	for (EGeoGemTier const Tier : {EGeoGemTier::Chip, EGeoGemTier::Cut, EGeoGemTier::Prism, EGeoGemTier::Core})
	{
		TestTrue(TEXT("Every tier has gems, a craft cost and a break-down value"),
				 !Catalog->GemsByTier.FindRef(Tier).Gems.IsEmpty() && Catalog->CraftCosts.Contains(Tier)
					 && Catalog->BreakDownShards.Contains(Tier));
	}
	for (EGeoDifficulty const Difficulty : {EGeoDifficulty::Safe, EGeoDifficulty::Reduced, EGeoDifficulty::Original})
	{
		TestTrue(TEXT("Every difficulty scales loot and XP"),
				 Catalog->DifficultyLootScale.Contains(Difficulty) && Catalog->DifficultyXpScale.Contains(Difficulty));
	}

	for (EGeoBossType const BossType : {EGeoBossType::Boss, EGeoBossType::MiniBoss})
	{
		FGeoGemDropTable const DropTable = Catalog->DropTables.FindRef(BossType);
		TestTrue(TEXT("Every boss type drops gems and gives XP"),
				 DropTable.MinCount > 0 && DropTable.MaxCount >= DropTable.MinCount
					 && Catalog->XpPerHealthBar.FindRef(BossType) > 0.f);
	}

	auto DropChance = [Catalog](EGeoBossType BossType, EGeoGemTier Tier)
	{
		return Catalog->DropTables.FindRef(BossType).TierChances.FindRef(Tier);
	};
	TestTrue(TEXT("Bosses drop Cuts"), DropChance(EGeoBossType::Boss, EGeoGemTier::Cut) > 0.f);
	TestTrue(TEXT("Bosses drop Prisms"), DropChance(EGeoBossType::Boss, EGeoGemTier::Prism) > 0.f);
	TestEqual(TEXT("Bosses never drop Cores"), DropChance(EGeoBossType::Boss, EGeoGemTier::Core), 0.f);
	TestTrue(TEXT("Mini-bosses drop Cuts"), DropChance(EGeoBossType::MiniBoss, EGeoGemTier::Cut) > 0.f);
	TestEqual(TEXT("Mini-bosses never drop Prisms"), DropChance(EGeoBossType::MiniBoss, EGeoGemTier::Prism), 0.f);
	TestEqual(TEXT("Mini-bosses never drop Cores"), DropChance(EGeoBossType::MiniBoss, EGeoGemTier::Core), 0.f);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemClassXpTest, "GeoTrinity.Gems.ClassXp", GeoGemTestFlags)

bool FGeoGemClassXpTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();

	TestEqual(TEXT("A whole health bar on Original"), Catalog.GetXp(EGeoBossType::Boss, 1.f, EGeoDifficulty::Original),
			  400);
	TestEqual(TEXT("Half a health bar, boss alive"), Catalog.GetXp(EGeoBossType::Boss, 0.5f, EGeoDifficulty::Original),
			  200);
	TestEqual(TEXT("Reduced scales XP"), Catalog.GetXp(EGeoBossType::Boss, 1.f, EGeoDifficulty::Reduced), 300);
	TestEqual(TEXT("Safe scales XP"), Catalog.GetXp(EGeoBossType::Boss, 1.f, EGeoDifficulty::Safe), 200);
	TestEqual(TEXT("No more than one health bar"), Catalog.GetXp(EGeoBossType::Boss, 1.5f, EGeoDifficulty::Original),
			  400);
	TestEqual(TEXT("No boss health taken, no XP"), Catalog.GetXp(EGeoBossType::Boss, 0.f, EGeoDifficulty::Original), 0);
	TestEqual(TEXT("A mini-boss bar has its own worth"),
			  Catalog.GetXp(EGeoBossType::MiniBoss, 0.5f, EGeoDifficulty::Original),
			  FMath::RoundToInt32(Catalog.XpPerHealthBar.FindRef(EGeoBossType::MiniBoss) * 0.5f));

	TestEqual(TEXT("Just short of a level"), Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 999), 0);
	TestEqual(TEXT("XP kept towards the next level"), Profile->GetClassXp(EPlayerClass::Triangle), 999);
	TestEqual(TEXT("One more XP levels up"), Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 1), 1);
	TestEqual(TEXT("Level 2"), Profile->GetClassLevel(EPlayerClass::Triangle), 2);
	TestEqual(TEXT("Leftover XP carries over"), Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 2500), 2);
	TestEqual(TEXT("Level 4 with 500 XP"), Profile->GetClassXp(EPlayerClass::Triangle), 500);
	TestEqual(TEXT("Other classes level on their own"), Profile->GetClassLevel(EPlayerClass::Circle), 1);

	Profile->AddClassXp(Catalog, EPlayerClass::Triangle, 1000000);
	TestEqual(TEXT("Level stops at the max"), Profile->GetClassLevel(EPlayerClass::Triangle), GeoGem::MaxClassLevel);
	TestEqual(TEXT("No XP kept at the max"), Profile->GetClassXp(EPlayerClass::Triangle), 0);

	int32 const XpToMax = (GeoGem::MaxClassLevel - 1) * Catalog.XpPerLevel;
	Profile->AddClassXp(Catalog, EPlayerClass::Square, XpToMax - 1);
	TestEqual(TEXT("Linear: one XP short of the max is level 19"), Profile->GetClassLevel(EPlayerClass::Square), 19);
	TestEqual(TEXT("47.5 Original health bars to level 20, about 10 hours at five kills an hour"),
			  static_cast<float>(XpToMax) / Catalog.XpPerHealthBar.FindRef(EGeoBossType::Boss), 47.5f);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemLootTest, "GeoTrinity.Gems.Loot", GeoGemTestFlags)

bool FGeoGemLootTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	FRandomStream Stream(1234);

	bool bOnlyCatalogGems = true;
	auto CountTiers = [&Catalog, &bOnlyCatalogGems](TArray<FGeoGemStack> const& Loot)
	{
		TMap<EGeoGemTier, int32> Counts;
		for (FGeoGemStack const& Stack : Loot)
		{
			TOptional<EGeoGemTier> const Tier = Catalog.FindTier(Stack.Id);
			bOnlyCatalogGems &= Tier.IsSet();
			if (Tier)
			{
				Counts.FindOrAdd(*Tier) += Stack.Count;
			}
		}
		return Counts;
	};

	auto CountAll = [](TMap<EGeoGemTier, int32> const& Counts)
	{
		int32 Total = 0;
		for (TPair<EGeoGemTier, int32> const& Tier : Counts)
		{
			Total += Tier.Value;
		}
		return Total;
	};

	constexpr int32 RollCount = 2000;
	bool bCountsInRange = true;
	int32 TotalCuts = 0;
	int32 TotalPrisms = 0;
	TSet<FName> DroppedGems;
	for (int32 Roll = 0; Roll < RollCount; ++Roll)
	{
		TArray<FGeoGemStack> const Loot = Catalog.RollLoot(EGeoBossType::Boss, EGeoDifficulty::Original, Stream);
		TMap<EGeoGemTier, int32> const Counts = CountTiers(Loot);
		bCountsInRange &= CountAll(Counts) >= 38 && CountAll(Counts) <= 50 && !Counts.Contains(EGeoGemTier::Core);
		TotalCuts += Counts.FindRef(EGeoGemTier::Cut);
		TotalPrisms += Counts.FindRef(EGeoGemTier::Prism);
		for (FGeoGemStack const& Stack : Loot)
		{
			DroppedGems.Add(Stack.Id);
		}
	}
	TestTrue(TEXT("Only catalog gems drop"), bOnlyCatalogGems);
	TestTrue(TEXT("Boss kill: 38-50 gems, no Core"), bCountsInRange);
	TestEqual(TEXT("Boss kill: 3% of 44 gems are Prisms"), static_cast<float>(TotalPrisms) / RollCount, 1.32f, 0.08f);
	TestEqual(TEXT("Boss kill: 16% of the rest are Cuts"), static_cast<float>(TotalCuts) / RollCount, 6.83f, 0.2f);
	TestEqual(TEXT("Every Chip, Cut and Prism type drops"), DroppedGems.Num(), 5);

	bool bMiniBossInRange = true;
	bool bSafeInRange = true;
	for (int32 Roll = 0; Roll < RollCount; ++Roll)
	{
		TMap<EGeoGemTier, int32> const MiniBoss =
			CountTiers(Catalog.RollLoot(EGeoBossType::MiniBoss, EGeoDifficulty::Original, Stream));
		bMiniBossInRange &= CountAll(MiniBoss) >= 38 && CountAll(MiniBoss) <= 50
			&& !MiniBoss.Contains(EGeoGemTier::Prism) && !MiniBoss.Contains(EGeoGemTier::Core);

		int32 const SafeCount = CountAll(CountTiers(Catalog.RollLoot(EGeoBossType::Boss, EGeoDifficulty::Safe, Stream)));
		bSafeInRange &= SafeCount >= 19 && SafeCount <= 25;
	}
	TestTrue(TEXT("Mini-boss kill: 38-50 gems, no Prism, no Core"), bMiniBossInRange);
	TestTrue(TEXT("Safe halves the count: 19-25 gems"), bSafeInRange);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemStatsTest, "GeoTrinity.Gems.CharacterStats", GeoGemTestFlags)

bool FGeoGemStatsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);

	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMaxAmmoAttribute(), 30.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute(), 1.f);

	auto Value = [ASC](FGameplayAttribute const& Attribute)
	{
		return ASC->GetNumericAttribute(Attribute);
	};

	FGeoGemLoadout Loadout;
	Loadout.Sockets.Init(NAME_None, GeoGem::GetSockets().Num());
	for (int32 i = 0; i < 20; ++i)
	{
		Loadout.Sockets[i] = "Power";
	}
	for (int32 i = 20; i < 30; ++i)
	{
		Loadout.Sockets[i] = "Vigor";
	}
	for (int32 i = 30; i < 45; ++i)
	{
		Loadout.Sockets[i] = "Magazine";
	}
	Loadout.Sockets[45] = "Surplus";

	FActiveGameplayEffectHandle Handle =
		ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestTrue(TEXT("Gem effect applied"), Handle.IsValid());
	TestEqual(TEXT("20 Power: damage +4%"), Value(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.04f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("10 Vigor: max health +3.5%"), Value(UCharacterAttributeSet::GetMaxHealthAttribute()), 103.5f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("15 Magazine: max ammo +18%"), Value(UCharacterAttributeSet::GetMaxAmmoAttribute()), 35.4f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Guard: damage reduction untouched"), Value(UCharacterAttributeSet::GetDamageReductionAttribute()),
			  0.f);
	TestEqual(TEXT("No Swift: speed untouched"), Value(UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute()),
			  1.f);

	ASC->RemoveActiveGameplayEffect(Handle);
	for (int32 i = 0; i < 30; ++i)
	{
		Loadout.Sockets[i] = "Guard";
	}
	Handle = ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestEqual(TEXT("Respec: damage back to base"), Value(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Respec: max health back to base"), Value(UCharacterAttributeSet::GetMaxHealthAttribute()), 100.f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("30 Guard: damage reduction +6%"), Value(UCharacterAttributeSet::GetDamageReductionAttribute()),
			  0.06f, KINDA_SMALL_NUMBER);

	ASC->RemoveActiveGameplayEffect(Handle);
	TestEqual(TEXT("Removing the effect clears every gem stat"),
			  Value(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Removing the effect restores max ammo"), Value(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.f,
			  KINDA_SMALL_NUMBER);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
