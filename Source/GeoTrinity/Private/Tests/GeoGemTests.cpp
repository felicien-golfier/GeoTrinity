// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/GeoGemDrop.h"
#include "Algo/Count.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemMeshes.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "Misc/AutomationTest.h"
#include "Settings/GameDataSettings.h"
#include "Tests/GeoTestDeployable.h"
#include "Tests/GeoTestGemDrop.h"
#include "Tests/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

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

/** Three Chips, a Cut, a Prism and two Cores, with the doc's drop tables and 1000 XP per echo level, free of any
 *  asset. */
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
	Catalog->GemsByTier[EGeoGemTier::Prism].Gems = {
		MakeGem("Anchor", UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute(), EGeoGemOperation::Add, 0.03f)};
	FGeoGemInfo Critical =
		MakeGem("Critical", UGeoGemAttributeSet::GetCritDamageAttribute(), EGeoGemOperation::Add, 0.3f);
	Critical.GrantedTag = FGeoGameplayTags::Get().Gem_Core_Critical;
	Catalog->GemsByTier[EGeoGemTier::Core].Gems = {MakeGem("Surplus"), Critical};

	FGeoGemDropTable& BossDrops = Catalog->DropTables.Add(EGeoBossType::Boss);
	BossDrops.MinCount = 38;
	BossDrops.MaxCount = 50;
	BossDrops.TierChances = {{EGeoGemTier::Prism, 0.03f}, {EGeoGemTier::Cut, 0.16f}};
	FGeoGemDropTable& MiniBossDrops = Catalog->DropTables.Add(EGeoBossType::MiniBoss);
	MiniBossDrops.MinCount = 38;
	MiniBossDrops.MaxCount = 50;
	MiniBossDrops.TierChances = {{EGeoGemTier::Cut, 0.16f}};

	UCurveFloat* EchoXpCurve = NewObject<UCurveFloat>(Catalog);
	EchoXpCurve->FloatCurve.AddKey(2.f, 1000.f);
	Catalog->EchoXpCurve = EchoXpCurve;
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
	TestTrue(TEXT("Each cluster's Core opens last"),
			 FindSocket(EGeoGemTier::Core, 7) != INDEX_NONE && FindSocket(EGeoGemTier::Core, 14) != INDEX_NONE
				 && FindSocket(EGeoGemTier::Core, 20) != INDEX_NONE);
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
	TestEqual(TEXT("No copy says why"),
			  Profile->GetEquipRefusal(Catalog, EPlayerClass::Triangle, LevelOneChip, "Power"),
			  EGeoGemRefusal::NoFreeCopy);

	Profile->AddGems("Power", 3);
	TestFalse(TEXT("Socket locked above the class level"),
			  Profile->Equip(Catalog, EPlayerClass::Triangle, LevelTwoChip, "Power"));
	TestEqual(TEXT("A locked socket says why"),
			  Profile->GetEquipRefusal(Catalog, EPlayerClass::Triangle, LevelTwoChip, "Power"), EGeoGemRefusal::Locked);
	TestFalse(TEXT("Unknown gem"), Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Nothing"));
	TestEqual(TEXT("A gem of another tier says why"),
			  Profile->GetEquipRefusal(Catalog, EPlayerClass::Triangle, FindSocket(EGeoGemTier::Core, 7), "Power"),
			  EGeoGemRefusal::WrongTier);

	TestEqual(TEXT("Fill at level 1 takes both open Chip sockets"),
			  Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Power"), 2);
	TestEqual(TEXT("Slotted copies are taken from the class's free count"),
			  Profile->GetFreeCount("Power", EPlayerClass::Triangle), 1);
	TestEqual(TEXT("Another class slots the same copies"), Profile->FillSockets(Catalog, EPlayerClass::Circle, "Power"),
			  2);
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
			  Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Power"), 2);

	Profile->SetClassLevel(EPlayerClass::Triangle, 1);
	TestEqual(TEXT("Lowering the level empties the sockets locked again"),
			  static_cast<int32>(Algo::CountIf(Profile->GetLoadout(EPlayerClass::Triangle).Sockets,
											   [](FName const GemId)
											   {
												   return !GemId.IsNone();
											   })),
			  2);
	TestEqual(TEXT("Fill replaces slotted gems while copies are free"),
			  Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Guard"), 1);
	Profile->SetClassLevel(EPlayerClass::Triangle, GeoGem::MaxClassLevel);
	Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Power");

	Profile->AddGems("Surplus", 2);
	TestEqual(TEXT("A Core fills one socket at most"), Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Surplus"),
			  1);
	TestEqual(TEXT("Fill of a held Core slots none"), Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Surplus"),
			  0);
	int32 const HeldCoreSocket = Profile->GetLoadout(EPlayerClass::Triangle).Sockets.Find("Surplus");
	TestFalse(TEXT("A Core is refused in its own socket"),
			  Profile->Equip(Catalog, EPlayerClass::Triangle, HeldCoreSocket, "Surplus"));
	TestEqual(TEXT("A gem already in the socket says why"),
			  Profile->GetEquipRefusal(Catalog, EPlayerClass::Triangle, HeldCoreSocket, "Surplus"),
			  EGeoGemRefusal::AlreadyThere);
	int32 const OtherCoreSocket =
		FindSocket(EGeoGemTier::Core, HeldCoreSocket == FindSocket(EGeoGemTier::Core, 7) ? 14 : 7);
	TestTrue(TEXT("A held Core moves to another socket"),
			 Profile->Equip(Catalog, EPlayerClass::Triangle, OtherCoreSocket, "Surplus"));
	TestTrue(TEXT("The previous socket emptied"),
			 Profile->GetLoadout(EPlayerClass::Triangle).Sockets[HeldCoreSocket].IsNone());
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemUnequipTierTest, "GeoTrinity.Gems.UnequipTier", GeoGemTestFlags)

bool FGeoGemUnequipTierTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	EPlayerClass const PlayerClass = EPlayerClass::Triangle;
	Profile->SetClassLevel(PlayerClass, GeoGem::MaxClassLevel);
	Profile->AddGems("Power", 40);
	Profile->AddGems("Magazine", 20);
	Profile->AddGems("Surplus", 1);
	Profile->FillSockets(Catalog, PlayerClass, "Power");
	Profile->FillSockets(Catalog, PlayerClass, "Magazine");
	Profile->FillSockets(Catalog, PlayerClass, "Surplus");
	auto const CountSlotted = [Profile, PlayerClass](FName const GemId)
	{
		return static_cast<int32>(Algo::Count(Profile->GetLoadout(PlayerClass).Sockets, GemId));
	};
	int32 const Cuts = CountSlotted("Magazine");
	TestTrue(TEXT("Cuts are slotted"), Cuts > 0);

	Profile->UnequipTier(Catalog, PlayerClass, EGeoGemTier::Cut);
	TestEqual(TEXT("The tier's sockets empty"), CountSlotted("Magazine"), 0);
	TestTrue(TEXT("Other tiers stay"), CountSlotted("Power") > 0 && CountSlotted("Surplus") == 1);
	TestEqual(TEXT("The copies go back to free"), Profile->GetFreeCount("Magazine", PlayerClass), 20);

	Profile->SetEcho(Catalog, PlayerClass, "Power");
	Profile->UnequipTier(Catalog, PlayerClass, EGeoGemTier::Cut);
	TestEqual(TEXT("The echo of another tier stays"), Profile->GetEcho(PlayerClass), FName("Power"));
	Profile->UnequipTier(Catalog, PlayerClass, EGeoGemTier::Chip);
	TestTrue(TEXT("The echo of the tier empties"), Profile->GetEcho(PlayerClass).IsNone());
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemBuildsTest, "GeoTrinity.Gems.Builds", GeoGemTestFlags)

bool FGeoGemBuildsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	int32 const LevelOneChip = FindSocket(EGeoGemTier::Chip, 1);
	auto const CountPlayed = [Profile](FName const GemId)
	{
		return static_cast<int32>(Algo::Count(Profile->GetLoadout(EPlayerClass::Triangle).Sockets, GemId));
	};

	Profile->AddGems("Power", 3);
	Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Power");
	TestTrue(TEXT("A build is added"), Profile->AddBuild(EPlayerClass::Triangle));
	TestEqual(TEXT("The new build is played"), Profile->GetActiveBuild(EPlayerClass::Triangle), 1);
	TestEqual(TEXT("The new build starts empty"), CountPlayed(NAME_None), GeoGem::GetSockets().Num());
	TestEqual(TEXT("Every copy is free in the new build"), Profile->GetFreeCount("Power", EPlayerClass::Triangle), 3);

	Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Power");
	TestEqual(TEXT("Builds share copies: breaking down keeps the most one build slots"),
			  Profile->GetBreakableCount("Power"), 2);

	Profile->RenameActiveBuild(EPlayerClass::Triangle, TEXT("  A very long build name  "));
	TestEqual(TEXT("A name is trimmed and cut"), Profile->GetBuilds(EPlayerClass::Triangle)[1].Name,
			  FString(TEXT("A very long bu")));

	Profile->RemoveActiveBuild(EPlayerClass::Triangle);
	TestEqual(TEXT("Removing a build plays the one before"), Profile->GetActiveBuild(EPlayerClass::Triangle), 0);
	TestEqual(TEXT("The first build kept its gem"), CountPlayed("Power"), 1);
	Profile->RemoveActiveBuild(EPlayerClass::Triangle);
	TestEqual(TEXT("The last build stays"), Profile->GetBuilds(EPlayerClass::Triangle).Num(), 1);

	for (int32 BuildIndex = 1; BuildIndex < GeoGem::MaxBuilds; ++BuildIndex)
	{
		Profile->AddBuild(EPlayerClass::Triangle);
	}

	TestFalse(TEXT("No build past the max"), Profile->AddBuild(EPlayerClass::Triangle));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemForgeTest, "GeoTrinity.Gems.Forge", GeoGemTestFlags)

bool FGeoGemForgeTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();

	Profile->AddGems("Power", 4);
	Profile->FillSockets(Catalog, EPlayerClass::Triangle, "Power");
	TestEqual(TEXT("Breaking down never touches slotted copies"), Profile->BreakDown(Catalog, "Power", 10), 2);
	TestEqual(TEXT("Slotted copies stay owned"), Profile->GetOwnedCount("Power"), 2);
	Profile->FillSockets(Catalog, EPlayerClass::Circle, "Power");
	TestEqual(TEXT("Copies slotted in two classes are never broken down"), Profile->BreakDown(Catalog, "Power", 10), 0);
	TestEqual(TEXT("Shards from two Chips"), Profile->GetShards(), 2);

	Profile->AddGems("Guard", 10);
	Profile->AddGems("Vigor", 3);
	Profile->AddGems("Magazine", 2);
	TestEqual(TEXT("Breaking a tier down takes every free gem of it"),
			  Profile->BreakDownTier(Catalog, EGeoGemTier::Chip), 13);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemProfileEventsTest, "GeoTrinity.Gems.ProfileEvents", GeoGemTestFlags)

bool FGeoGemProfileEventsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	int32 Equipped = 0;
	int32 LastSocket = INDEX_NONE;
	FName LastEquippedGem;
	Profile->OnGemEquipped.AddLambda(
		[&](EPlayerClass /*PlayerClass*/, int32 const SocketIndex, FName const GemId)
		{
			++Equipped;
			LastSocket = SocketIndex;
			LastEquippedGem = GemId;
		});
	int32 Crafted = 0;
	int32 LastQuantity = 0;
	Profile->OnGemCrafted.AddLambda(
		[&](FName /*GemId*/, int32 const Quantity)
		{
			++Crafted;
			LastQuantity = Quantity;
		});

	int32 const LevelOneChip = FindSocket(EGeoGemTier::Chip, 1);
	Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Power");
	TestEqual(TEXT("A refused Equip broadcasts nothing"), Equipped, 0);
	Profile->AddGems("Power", 3);
	Profile->Equip(Catalog, EPlayerClass::Triangle, LevelOneChip, "Power");
	TestEqual(TEXT("A successful Equip broadcasts once"), Equipped, 1);
	TestEqual(TEXT("With its socket"), LastSocket, LevelOneChip);
	TestEqual(TEXT("And its gem"), LastEquippedGem, FName("Power"));
	Profile->FillSockets(Catalog, EPlayerClass::Circle, "Power");
	TestEqual(TEXT("FillSockets broadcasts once per socket filled"), Equipped, 3);

	Profile->Craft(Catalog, "Guard", 1);
	TestEqual(TEXT("A refused Craft broadcasts nothing"), Crafted, 0);
	Profile->AddShards(10);
	Profile->Craft(Catalog, "Guard", 2);
	TestEqual(TEXT("A successful Craft broadcasts once"), Crafted, 1);
	TestEqual(TEXT("With its quantity"), LastQuantity, 2);
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
			TestFalse(FString::Printf(TEXT("%s: every {Token} of its effect text resolves"), *Gem.Id.ToString()),
					  Gem.GetEffectText().ToString().Contains(TEXT("{")));
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

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	for (TPair<FName, FGameplayTag> const& Core :
		 TArray<TPair<FName, FGameplayTag>>{{"Critical", Tags.Gem_Core_Critical}, {"Leverage", Tags.Gem_Core_Leverage}})
	{
		FGeoGemInfo const* Gem = Catalog->Find(Core.Key);
		TestTrue(FString::Printf(TEXT("%s grants the tag its rule checks"), *Core.Key.ToString()),
				 Gem && Gem->GrantedTag == Core.Value);
	}

	UAbilityInfo const* AbilityInfo = GeoASLib::GetAbilityInfo();
	for (FName const CoreId : {"Relay", "Bond", "Wake", "Split", "Kinship"})
	{
		FGeoGemInfo const* Gem = Catalog->Find(CoreId);
		TestTrue(FString::Printf(TEXT("%s gives a Core passive"), *CoreId.ToString()), Gem && Gem->GrantedAbility);
		TestTrue(
			FString::Printf(TEXT("%s's passive is in the ability catalog, for the abilities page"), *CoreId.ToString()),
			Gem && AbilityInfo
				&& AbilityInfo->GetAllPlayersAbilityInfos().ContainsByPredicate(
					[Gem](FPlayersGameplayAbilityInfo const& Info)
					{
						return Info.AbilityClass == Gem->GrantedAbility && !Info.bGiveAtStartup;
					}));
	}
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

	int32 XpTaken = 0;
	int32 XpNeeded = 0;
	TestEqual(TEXT("Bar: 500 of 1000 into level 4"),
			  Profile->GetXpProgress(Catalog, EPlayerClass::Triangle, XpTaken, XpNeeded), 0.5f);
	TestEqual(TEXT("Bar numbers below the max"), XpTaken, 500);
	TestEqual(TEXT("Bar needs one level of XP"), XpNeeded, Catalog.XpPerLevel);

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemEchoTest, "GeoTrinity.Gems.Echo", GeoGemTestFlags)

bool FGeoGemEchoTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	EPlayerClass const PlayerClass = EPlayerClass::Triangle;
	Profile->AddGems("Power", 2);
	Profile->AddGems("Guard", 1);
	Profile->AddGems("Surplus", 1);

	TestEqual(TEXT("Echo level starts at 0"), Profile->GetEchoLevel(), 0);
	TestFalse(TEXT("Echo locked while no class reached 20"), Profile->CanEcho(Catalog, PlayerClass, "Power"));
	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel);
	TestEqual(TEXT("The first class at 20 takes the echo to 1"), Profile->GetEchoLevel(), 1);
	TestTrue(TEXT("Echo open at level 20"), Profile->CanEcho(Catalog, PlayerClass, "Power"));
	TestTrue(TEXT("Echo open on a class at level 1"), Profile->CanEcho(Catalog, EPlayerClass::Circle, "Power"));
	Profile->SetClassLevel(EPlayerClass::Circle, GeoGem::EchoUnlockLevel);
	TestEqual(TEXT("A second class at 20 keeps it"), Profile->GetEchoLevel(), 1);
	Profile->SetClassLevel(EPlayerClass::Circle, 1);
	TestFalse(TEXT("A Core is refused"), Profile->CanEcho(Catalog, PlayerClass, "Surplus"));
	TestFalse(TEXT("A gem with no copy is refused"), Profile->CanEcho(Catalog, PlayerClass, "Vigor"));
	TestFalse(TEXT("An unknown gem is refused"), Profile->CanEcho(Catalog, PlayerClass, "Nothing"));
	TestEqual(TEXT("A Core says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Surplus"),
			  EGeoGemRefusal::EchoCore);
	TestEqual(TEXT("No copy says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Vigor"), EGeoGemRefusal::NoCopy);
	TestEqual(TEXT("An open echo takes it"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Power"),
			  EGeoGemRefusal::None);
	Profile->SetClassLevel(PlayerClass, 1);
	TestEqual(TEXT("A locked echo says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Power"),
			  EGeoGemRefusal::EchoLocked);
	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel);

	TestEqual(TEXT("Both copies slotted"), Profile->FillSockets(Catalog, PlayerClass, "Power"), 2);
	TestTrue(TEXT("Echoing a slotted type"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("The echoed gem"), Profile->GetEcho(PlayerClass), FName("Power"));
	TestEqual(TEXT("Echoing empties the build's sockets of that type"),
			  static_cast<int32>(Algo::Count(Profile->GetLoadout(PlayerClass).Sockets, FName("Power"))), 0);
	TestEqual(TEXT("The echo takes one copy"), Profile->GetFreeCount("Power", PlayerClass), 1);
	TestEqual(TEXT("The echo reserves a copy from the Forge"), Profile->GetBreakableCount("Power"), 1);
	TestFalse(TEXT("Echoing the echoed gem again"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("Echoing the echoed gem says why"), Profile->GetEchoRefusal(Catalog, PlayerClass, "Power"),
			  EGeoGemRefusal::AlreadyEcho);
	TestTrue(TEXT("Slotting the echoed type"),
			 Profile->Equip(Catalog, PlayerClass, FindSocket(EGeoGemTier::Chip, 1), "Power"));
	TestTrue(TEXT("Slotting it empties the echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestEqual(TEXT("The echo's copy moved"), Profile->GetFreeCount("Power", PlayerClass), 1);
	TestTrue(TEXT("Echoing it again empties the socket"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("Fill of an echoed type clears the echo, then fills"),
			  Profile->FillSockets(Catalog, PlayerClass, "Power"), 2);
	TestTrue(TEXT("Fill emptied the echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestTrue(TEXT("Echo a slotted type, one copy owned"), Profile->SetEcho(Catalog, PlayerClass, "Power"));
	TestEqual(TEXT("Echoed copies count once"), Profile->GetFreeCount("Power", PlayerClass), 1);

	TestTrue(TEXT("Another gem replaces the echo"), Profile->SetEcho(Catalog, PlayerClass, "Guard"));
	TestEqual(TEXT("The replaced gem is free again"), Profile->GetFreeCount("Power", PlayerClass), 2);
	Profile->ClearEcho(PlayerClass);
	TestTrue(TEXT("Cleared echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestEqual(TEXT("Cleared echo gives its copy back"), Profile->GetFreeCount("Guard", PlayerClass), 1);

	Profile->SetEcho(Catalog, PlayerClass, "Power");
	TestEqual(TEXT("A class under 20 keeps its XP"), Profile->AddClassXp(Catalog, EPlayerClass::Circle, 10), 0);
	TestEqual(TEXT("Its XP goes to the class"), Profile->GetClassXp(EPlayerClass::Circle), 10);
	TestEqual(TEXT("Not to the echo"), Profile->GetEchoXp(), 0);
	TestEqual(TEXT("Max class level: XP raises the echo level"), Profile->AddClassXp(Catalog, PlayerClass, 3500), 0);
	TestEqual(TEXT("Echo level 4"), Profile->GetEchoLevel(), 4);
	TestEqual(TEXT("Echo XP towards the next"), Profile->GetEchoXp(), 500);
	int32 XpTaken = 0;
	int32 XpNeeded = 0;
	float const Fill = Profile->GetXpProgress(Catalog, PlayerClass, XpTaken, XpNeeded);
	TestEqual(TEXT("The bar carries on with the echo XP"), XpTaken, 500);
	TestEqual(TEXT("The bar needs the next echo level"), XpNeeded, Catalog.GetEchoXpForLevel(5));
	TestEqual(TEXT("Bar fill is echo XP over its cost"), Fill, 500.f / XpNeeded, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("The class level stays"), Profile->GetClassLevel(PlayerClass), GeoGem::MaxClassLevel);
	TestEqual(TEXT("The loadout carries the echo"), Profile->GetLoadout(PlayerClass).Echo, FName("Power"));
	TestEqual(TEXT("The loadout carries the echo level"), Profile->GetLoadout(PlayerClass).EchoLevel, 4);
	TestEqual(TEXT("A class under 20 sees the shared level"), Profile->GetLoadout(EPlayerClass::Circle).EchoLevel, 4);

	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel - 1);
	TestTrue(TEXT("No class at 20 empties the echo"), Profile->GetEcho(PlayerClass).IsNone());
	TestEqual(TEXT("No class at 20 resets the echo level"), Profile->GetEchoLevel(), 0);
	TestEqual(TEXT("And its XP"), Profile->GetEchoXp(), 0);
	TestFalse(TEXT("Echo locked again"), Profile->CanEcho(Catalog, PlayerClass, "Power"));

	Profile->SetClassLevel(PlayerClass, GeoGem::EchoUnlockLevel);
	Profile->AddClassXp(Catalog, PlayerClass, 1000000);
	TestEqual(TEXT("Echo level stops at the max"), Profile->GetEchoLevel(), Catalog.EchoMaxLevel);
	int32 const KeptXp = Profile->GetEchoXp();
	Profile->AddClassXp(Catalog, PlayerClass, 123);
	TestEqual(TEXT("Every echo XP point is kept"), Profile->GetEchoXp(), KeptXp + 123);

	Profile->SetClassLevel(PlayerClass, 1);
	Profile->SetEchoLevel(Catalog, 5);
	TestEqual(TEXT("An echo level needs a class at 20"), Profile->GetClassLevel(EPlayerClass::Triangle),
			  GeoGem::EchoUnlockLevel);
	TestEqual(TEXT("The echo level is set"), Profile->GetEchoLevel(), 5);

	Profile->SetEchoLevel(Catalog, 1000);
	TestEqual(TEXT("Echo level clamps to the max"), Profile->GetEchoLevel(), Catalog.EchoMaxLevel);
	Profile->SetEchoLevel(Catalog, -5);
	TestEqual(TEXT("Echo level clamps to 0"), Profile->GetEchoLevel(), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemEchoStatsTest, "GeoTrinity.Gems.EchoStats", GeoGemTestFlags)

bool FGeoGemEchoStatsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	TestEqual(TEXT("Level 0: no echo share"), Catalog.GetEchoShare(EGeoGemTier::Chip, 0), 0.f);
	TestEqual(TEXT("Level 1: Chip 1%"), Catalog.GetEchoShare(EGeoGemTier::Chip, 1), 0.01f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 1: no Cut"), Catalog.GetEchoShare(EGeoGemTier::Cut, 1), 0.f);
	TestEqual(TEXT("Level 1: no Prism"), Catalog.GetEchoShare(EGeoGemTier::Prism, 1), 0.f);
	TestEqual(TEXT("Level 2 adds Cut"), Catalog.GetEchoShare(EGeoGemTier::Cut, 2), 0.01f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 3 adds Prism"), Catalog.GetEchoShare(EGeoGemTier::Prism, 3), 0.01f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 4: Chip 2%"), Catalog.GetEchoShare(EGeoGemTier::Chip, 4), 0.02f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 100: Chip 34%"), Catalog.GetEchoShare(EGeoGemTier::Chip, 100), 0.34f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 100: Cut 33%"), Catalog.GetEchoShare(EGeoGemTier::Cut, 100), 0.33f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Level 100: Prism 33%"), Catalog.GetEchoShare(EGeoGemTier::Prism, 100), 0.33f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("A Core has no share"), Catalog.GetEchoShare(EGeoGemTier::Core, 100), 0.f);
	TestEqual(TEXT("Echo XP from the curve"), Catalog.GetEchoXpForLevel(2), 1000);
	TestEqual(TEXT("Level 1: a Chip echo is worth 30 sockets x 1%"), Catalog.GetEchoCopies(EGeoGemTier::Chip, 1), 0.3f,
			  KINDA_SMALL_NUMBER);
	FGeoGemInfo const& Magazine = *Catalog.Find("Magazine");
	TestFalse(TEXT("A zero echo share still shows its value"),
			  Magazine.GetMagnitudeText(Catalog.GetEchoCopies(EGeoGemTier::Cut, 1)).IsEmpty());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetMaxAmmoAttribute(), 30.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f);

	FGeoGemLoadout Loadout;
	Loadout.Sockets.Init(NAME_None, GeoGem::GetSockets().Num());
	auto Apply = [&](FName const Echo, int32 const EchoLevel)
	{
		Loadout.Echo = Echo;
		Loadout.EchoLevel = EchoLevel;
		return ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	};

	FActiveGameplayEffectHandle Handle = Apply("Power", 1);
	TestEqual(TEXT("Echoed Chip: 30 sockets x 0.2% x 1%"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.0006f,
			  KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Power", 100);
	TestEqual(TEXT("Echoed Chip at level 100: 34% of 6%"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.0204f,
			  KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Magazine", 1);
	TestEqual(TEXT("Echoed Cut at level 1: no share yet"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.f, KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Magazine", 2);
	TestEqual(TEXT("Echoed Cut at level 2: 15 sockets x 1.2% x 1%"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.054f, KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	Handle = Apply("Anchor", 3);
	TestEqual(TEXT("Echoed Prism at level 3: 9 sockets x 3% x 1%"),
			  ASC->GetNumericAttribute(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute()), 1.0027f,
			  KINDA_SMALL_NUMBER);
	ASC->RemoveActiveGameplayEffect(Handle);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
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
		TArray<FGeoGemStack> const Loot =
			Catalog.RollLoot(Catalog.DropTables.FindChecked(EGeoBossType::Boss), EGeoDifficulty::Original, Stream);
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
		TMap<EGeoGemTier, int32> const MiniBoss = CountTiers(
			Catalog.RollLoot(Catalog.DropTables.FindChecked(EGeoBossType::MiniBoss), EGeoDifficulty::Original, Stream));
		bMiniBossInRange &= CountAll(MiniBoss) >= 38 && CountAll(MiniBoss) <= 50
			&& !MiniBoss.Contains(EGeoGemTier::Prism) && !MiniBoss.Contains(EGeoGemTier::Core);

		int32 const SafeCount = CountAll(CountTiers(
			Catalog.RollLoot(Catalog.DropTables.FindChecked(EGeoBossType::Boss), EGeoDifficulty::Safe, Stream)));
		bSafeInRange &= SafeCount >= 19 && SafeCount <= 25;
	}
	TestTrue(TEXT("Mini-boss kill: 38-50 gems, no Prism, no Core"), bMiniBossInRange);
	TestTrue(TEXT("Safe halves the count: 19-25 gems"), bSafeInRange);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
/** Gems of a reward, all stacks summed. */
static int32 CountGems(FGeoGemReward const& Reward)
{
	int32 Total = 0;
	for (FGeoGemStack const& Stack : Reward.Gems)
	{
		Total += Stack.Count;
	}
	return Total;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBossLootDefaultTest, "GeoTrinity.Gems.BossLoot.Default", GeoGemTestFlags)

bool FGeoBossLootDefaultTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	FRandomStream Stream(77);
	FGeoBossLoot const BossLoot;

	bool bInRange = true;
	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		FGeoGemReward const Loot = Catalog.MakeKillLoot(BossLoot, EGeoBossType::Boss, EGeoDifficulty::Original, Stream);
		bInRange &= CountGems(Loot) >= 38 && CountGems(Loot) <= 50 && Loot.Shards == 0;
	}
	TestTrue(TEXT("Override off: the catalog's 38-50 gems, no shards"), bInRange);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBossLootOverrideTest, "GeoTrinity.Gems.BossLoot.Override", GeoGemTestFlags)

bool FGeoBossLootOverrideTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	FRandomStream Stream(78);
	FGeoBossLoot BossLoot;
	BossLoot.bOverrideDropTable = true;
	BossLoot.FixedGems = {{"Power", 3}, {"Guard", 1}};
	BossLoot.FixedShards = 5;

	FGeoGemReward const Loot = Catalog.MakeKillLoot(BossLoot, EGeoBossType::MiniBoss, EGeoDifficulty::Original, Stream);
	TestEqual(TEXT("Count 0 table: only the fixed gems"), CountGems(Loot), 4);
	TestEqual(TEXT("Fixed shards"), Loot.Shards, 5);
	TestEqual(TEXT("Two stacks"), Loot.Gems.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBossLootFixedUnscaledTest, "GeoTrinity.Gems.BossLoot.FixedUnscaled",
								 GeoGemTestFlags)

bool FGeoBossLootFixedUnscaledTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	FRandomStream Stream(79);
	FGeoBossLoot BossLoot;
	BossLoot.bOverrideDropTable = true;
	BossLoot.FixedGems = {{"Power", 3}};
	BossLoot.FixedShards = 5;
	FGeoGemReward const Fixed = Catalog.MakeKillLoot(BossLoot, EGeoBossType::Boss, EGeoDifficulty::Safe, Stream);
	TestEqual(TEXT("Safe: fixed gems unchanged"), CountGems(Fixed), 3);
	TestEqual(TEXT("Safe: fixed shards unchanged"), Fixed.Shards, 5);

	BossLoot.DropTable.MinCount = 10;
	BossLoot.DropTable.MaxCount = 10;
	bool bMerged = true;
	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		FGeoGemReward const Loot = Catalog.MakeKillLoot(BossLoot, EGeoBossType::Boss, EGeoDifficulty::Original, Stream);
		int32 PowerStacks = 0;
		for (FGeoGemStack const& Stack : Loot.Gems)
		{
			PowerStacks += Stack.Id == "Power" ? 1 : 0;
		}
		bMerged &= CountGems(Loot) == 13 && PowerStacks == 1;
	}
	TestTrue(TEXT("A fixed gem also rolled merges into one stack: 10 rolled + 3 fixed"), bMerged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBossLootShardsTest, "GeoTrinity.Gems.BossLoot.Shards", GeoGemTestFlags)

bool FGeoBossLootShardsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemProfileSave* Profile = NewObject<UGeoGemProfileSave>();
	Profile->AddShards(7);
	Profile->AddShards(5);
	TestEqual(TEXT("Shards add up"), Profile->GetShards(), 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoBossLootValidationTest, "GeoTrinity.Gems.BossLoot.Validation", GeoGemTestFlags)

bool FGeoBossLootValidationTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();
	FGeoBossLoot BossLoot;
	BossLoot.FixedGems = {{"Power", 2}, {"Magazine", 1}};
	TestEqual(TEXT("Valid list: no error"), Catalog.FindBossLootErrors(BossLoot).Num(), 0);

	BossLoot.FixedGems = {{"Nope", 1}};
	TestEqual(TEXT("Unknown id"), Catalog.FindBossLootErrors(BossLoot).Num(), 1);

	BossLoot.FixedGems = {{"Power", 0}};
	TestEqual(TEXT("Count 0"), Catalog.FindBossLootErrors(BossLoot).Num(), 1);
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
	ASC->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(Character));
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
	for (int32 i = 45; i < 54; ++i)
	{
		Loadout.Sockets[i] = "Anchor";
	}
	Loadout.Sockets[54] = "Surplus";

	FActiveGameplayEffectHandle Handle =
		ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestTrue(TEXT("Gem effect applied"), Handle.IsValid());
	TestEqual(TEXT("20 Power: damage +4%"), Value(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.04f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("10 Vigor: max health +3.5%"), Value(UCharacterAttributeSet::GetMaxHealthAttribute()), 103.5f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("15 Magazine: max ammo +18%"), Value(UCharacterAttributeSet::GetMaxAmmoAttribute()), 35.4f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Guard: damage reduction untouched"),
			  Value(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f);
	TestEqual(TEXT("No Swift: speed untouched"), Value(UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute()),
			  1.f);
	TestEqual(TEXT("9 Anchor: deployable health +27%"),
			  Value(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute()), 1.27f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Precision: no base crit chance"), Value(UGeoGemAttributeSet::GetCritChanceAttribute()), 0.f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No Edge: base crit damage"), Value(UGeoGemAttributeSet::GetCritDamageAttribute()), 1.2f,
			  KINDA_SMALL_NUMBER);
	TestFalse(TEXT("No Critical Core: no crit tag"),
			  ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Critical));

	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 1.f);
	TestEqual(TEXT("Without Critical, a sure crit chance still crits for 120%"), GeoASLib::RollCritMultiplier(ASC),
			  1.2f, KINDA_SMALL_NUMBER);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 0.f);
	TestEqual(TEXT("Without Critical and no crit chance: no crit"), GeoASLib::RollCritMultiplier(ASC), 1.f);

	ASC->RemoveActiveGameplayEffect(Handle);
	Loadout.Sockets[54] = "Critical";
	Handle = ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);
	TestTrue(TEXT("Critical slotted: crit tag held"),
			 ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Critical));
	TestEqual(TEXT("Critical: crit chance +10%"), Value(UGeoGemAttributeSet::GetCritChanceAttribute()), 0.1f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Critical: crit damage +30%"), Value(UGeoGemAttributeSet::GetCritDamageAttribute()), 1.5f,
			  KINDA_SMALL_NUMBER);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 1.f);
	TestEqual(TEXT("Critical and a sure crit chance: every hit deals 150%"), GeoASLib::RollCritMultiplier(ASC), 1.5f,
			  KINDA_SMALL_NUMBER);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetCritChanceAttribute(), 0.f);

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
	TestFalse(TEXT("Removing the effect takes the Core's tag"),
			  ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Gem_Core_Critical));
	TestEqual(TEXT("Removing the effect clears every gem stat"),
			  Value(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Removing the effect restores max ammo"), Value(UCharacterAttributeSet::GetMaxAmmoAttribute()), 30.f,
			  KINDA_SMALL_NUMBER);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetDashCooldownMultiplierAttribute(), 0.8f);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetDeployableCooldownMultiplierAttribute(), 0.85f);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetWindUpMultiplierAttribute(), 0.9f);
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetReloadSpeedMultiplierAttribute(), 1.25f);
	FGameplayTagContainer const DashTags(Tags.Ability_Type_Dash);
	FGameplayTagContainer const DeployableTags(Tags.Ability_Type_Deployable);
	FGameplayTagContainer const BasicTags(Tags.Ability_Type_Basic);
	FGameplayTagContainer ReloadTags(Tags.Ability_Type_Basic);
	ReloadTags.AddTag(Tags.Ability_Type_Reload);
	TestEqual(TEXT("Dash cooldown reads the dash multiplier"), GeoASLib::GetGemCooldownMultiplier(ASC, DashTags), 0.8f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Deploy recharge reads the deployable multiplier"),
			  GeoASLib::GetGemCooldownMultiplier(ASC, DeployableTags), 0.85f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("A basic ability's cooldown is untouched"), GeoASLib::GetGemCooldownMultiplier(ASC, BasicTags), 1.f);
	TestEqual(TEXT("Wind-up scales a basic ability's fire delay"), GeoASLib::GetGemFireDelayMultiplier(ASC, BasicTags),
			  0.9f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Reload speed +25% makes the reload 80% as long"),
			  GeoASLib::GetGemFireDelayMultiplier(ASC, ReloadTags), 0.8f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No ASC: fire delay untouched"), GeoASLib::GetGemFireDelayMultiplier(nullptr, BasicTags), 1.f);

	float const GeneralSpellDistance = GetDefault<UGameDataSettings>()->GeneralSpellDistance;
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetSpellDistanceMultiplierAttribute(), 1.1f);
	TestEqual(TEXT("Reach +10% lengthens beams by 10%"), GeoASLib::GetSpellDistance(ASC), GeneralSpellDistance * 1.1f,
			  KINDA_SMALL_NUMBER);
	TestEqual(TEXT("No ASC: beams keep the general spell distance"), GeoASLib::GetSpellDistance(nullptr),
			  GeneralSpellDistance);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemStatsSurviveDeathTest, "GeoTrinity.Gems.StatsSurviveDeath", GeoGemTestFlags)

bool FGeoGemStatsSurviveDeathTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const& Catalog = *MakeTestCatalog();

	// A listen-server world: the death purge is server-only.
	UWorld* World = UWorld::CreateWorld(EWorldType::PIE, false);
	World->SetPlayInEditorInitialNetMode(NM_ListenServer);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::PIE);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UGeoAbilitySystemComponent* ASC = NewObject<UGeoAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f);

	FGeoGemLoadout Loadout;
	Loadout.Sockets.Init(NAME_None, GeoGem::GetSockets().Num());
	for (int32 i = 0; i < 20; ++i)
	{
		Loadout.Sockets[i] = "Power";
	}
	FActiveGameplayEffectHandle const StatsHandle =
		ASC->ApplyGameplayEffectSpecToSelf(*UGeoGemStatsEffect::MakeSpec(*ASC, Catalog, Loadout).Data);

	UGameplayEffect* OtherEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	OtherEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo& Modifier = OtherEffect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UCharacterAttributeSet::GetDamageReductionAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.5f));
	FActiveGameplayEffectHandle const OtherHandle =
		ASC->ApplyGameplayEffectToSelf(OtherEffect, 1.f, ASC->MakeEffectContext());

	UGameplayEffect* CooldownEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	CooldownEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FInheritedTagContainer CooldownTags;
	CooldownTags.AddTag(FGeoGameplayTags::Get().Ability_Cooldown);
	CooldownEffect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>().SetAndApplyTargetTagChanges(CooldownTags);
	ASC->ApplyGameplayEffectToSelf(CooldownEffect, 1.f, ASC->MakeEffectContext());
	TestTrue(TEXT("Cooldown tag held before death"),
			 ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Ability_Cooldown));

	ASC->HandleOwnerDeath({StatsHandle});

	TestTrue(TEXT("Gem stats effect survives death"), ASC->GetActiveGameplayEffect(StatsHandle) != nullptr);
	TestEqual(TEXT("Gem stats still modify the attribute"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageMultiplierAttribute()), 1.04f,
			  KINDA_SMALL_NUMBER);
	TestTrue(TEXT("An unrelated effect is purged"), ASC->GetActiveGameplayEffect(OtherHandle) == nullptr);
	TestEqual(TEXT("Its modifier is gone"),
			  ASC->GetNumericAttribute(UCharacterAttributeSet::GetDamageReductionAttribute()), 0.f, KINDA_SMALL_NUMBER);
	TestFalse(TEXT("The cooldown is cleared"), ASC->HasMatchingGameplayTag(FGeoGameplayTags::Get().Ability_Cooldown));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableStartHealthTest, "GeoTrinity.Deployable.StartHealth", GeoGemTestFlags)

bool FGeoDeployableStartHealthTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();

	APawn* Deployer = World->SpawnActor<AGeoTestDeployer>();
	auto const Spawn = [Deployer](float const StartHealthRatio)
	{
		FDeployableData Data;
		Data.Owner = Deployer;
		Data.Instigator = Deployer;
		Data.Params.StartHealthRatio = StartHealthRatio;
		Data.Params.LifeDrainMaxDuration = 10.f;
		return GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform::Identity);
	};
	auto const Attribute = [](AGeoDeployableBase const* Deployable, FGameplayAttribute const& Which)
	{
		return Deployable->GetAbilitySystemComponent()->GetNumericAttribute(Which);
	};

	AGeoDeployableBase const* Full = Spawn(1.f);
	AGeoDeployableBase const* Half = Spawn(0.5f);
	if (TestNotNull(TEXT("Both deployables spawn"), Full) && TestNotNull(TEXT("Both deployables spawn"), Half))
	{
		float const MaxHealth = Attribute(Full, UGeoAttributeSetBase::GetMaxHealthAttribute());
		TestTrue(TEXT("Default attributes gave a max health"), MaxHealth > 0.f);
		TestEqual(TEXT("Ratio 1 starts full"), Attribute(Full, UGeoAttributeSetBase::GetHealthAttribute()), MaxHealth);
		TestEqual(TEXT("Ratio 0.5 keeps the max health"),
				  Attribute(Half, UGeoAttributeSetBase::GetMaxHealthAttribute()), MaxHealth);
		TestEqual(TEXT("Ratio 0.5 starts at half"), Attribute(Half, UGeoAttributeSetBase::GetHealthAttribute()),
				  MaxHealth * 0.5f, KINDA_SMALL_NUMBER);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableStartHealthFromAbilityTest,
								 "GeoTrinity.Deployable.StartHealthFromAbility", GeoGemTestFlags)

bool FGeoDeployableStartHealthFromAbilityTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();

	AGeoTestDeployer* Deployer = World->SpawnActor<AGeoTestDeployer>();
	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(Deployer->GetAbilitySystemComponent());
	ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestDeployAbility::StaticClass(), 1));
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute(), 2.f);

	AGeoDeployableBase const* Copy = GeoASLib::SpawnDeployableFromAbility(*ASC, FVector::ZeroVector, 0.5f);
	if (TestNotNull(TEXT("The ability places the deployable"), Copy))
	{
		UAbilitySystemComponent const* CopyASC = Copy->GetAbilitySystemComponent();
		float const BaseMaxHealth = UGeoTestDeployableAttributes::MaxHealth;
		TestEqual(TEXT("The deployer's gems scale the max health (x2)"),
				  CopyASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()), BaseMaxHealth * 2.f);
		TestEqual(TEXT("Ratio 0.5 starts at half of that"),
				  CopyASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()), BaseMaxHealth,
				  KINDA_SMALL_NUMBER);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoRelaySnapDashTest, "GeoTrinity.Gems.RelaySnapDash", GeoGemTestFlags)

bool FGeoRelaySnapDashTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	FGeoGemInfo const* Relay = Catalog ? Catalog->Find("Relay") : nullptr;
	if (!TestTrue(TEXT("The catalog's Relay gives a Core passive"), Relay && Relay->GrantedAbility))
	{
		return false;
	}

	UWorld* World = CreateTestWorld();

	AGeoTestDeployer* Deployer = World->SpawnActor<AGeoTestDeployer>();
	NewObject<UGeoDeployableManagerComponent>(Deployer)->RegisterComponent();
	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(Deployer->GetAbilitySystemComponent());
	ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestDeployAbility::StaticClass(), 1));
	FGameplayAbilitySpecHandle const RelayHandle = ASC->GiveAbility(FGameplayAbilitySpec(Relay->GrantedAbility, 1));
	bool const bRelayActive = ASC->TryActivateAbility(RelayHandle);

	FDeployableData Data;
	Data.Owner = Deployer;
	Data.Instigator = Deployer;
	Data.Params.LifeDrainMaxDuration = 10.f;
	AGeoDeployableBase const* Deployable =
		GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform(FVector(300.f, 0.f, 0.f)));

	if (TestTrue(TEXT("Relay activates and the deployable spawns"), bRelayActive && Deployable))
	{
		FVector Direction = FVector::ZeroVector;
		float Distance = 0.f;
		auto const AimDash = [ASC, &Direction, &Distance](bool const bHasDirection, float const Yaw)
		{
			Direction = FRotator(0.f, Yaw, 0.f).Vector();
			Distance = 500.f;
			ASC->OnDashAiming.Broadcast(FVector::ZeroVector, bHasDirection, Direction, Distance);
		};

		AimDash(true, 10.f);
		TestTrue(TEXT("A dash 10 degrees off turns straight at the deployable"),
				 Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));
		TestEqual(TEXT("It stops on the deployable"), Distance, 300.f, 1.f);

		AimDash(true, 90.f);
		TestEqual(TEXT("A dash at 90 degrees keeps its direction"), Direction.Y, 1.0,
				  static_cast<double>(UE_KINDA_SMALL_NUMBER));
		TestEqual(TEXT("And its distance"), Distance, 500.f);

		AimDash(false, 90.f);
		TestTrue(TEXT("A dash from standing still heads for the deployable"),
				 Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));

		ASC->CancelAllAbilities();
		AimDash(true, 10.f);
		TestTrue(TEXT("The revive reset (CancelAllAbilities, also run by the owning client) keeps the snap"),
				 Direction.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));

		ASC->ClearAbility(RelayHandle);
		AimDash(true, 10.f);
		TestTrue(TEXT("Removing the Relay spec stops the snap"),
				 Direction.Equals(FRotator(0.f, 10.f, 0.f).Vector(), KINDA_SMALL_NUMBER));
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemMeshPathsTest, "GeoTrinity.Gems.MeshPaths", GeoGemTestFlags)

bool FGeoGemMeshPathsTest::RunTest(FString const& /*Parameters*/)
{
	TSet<FSoftObjectPath> Paths;
	for (EGeoGemTier const Tier : {EGeoGemTier::Chip, EGeoGemTier::Cut, EGeoGemTier::Prism, EGeoGemTier::Core})
	{
		Paths.Add(GeoGem::GetTierMesh(Tier).ToSoftObjectPath());
	}

	for (int32 Variant = 0; Variant < GeoGem::ShardVariantCount; ++Variant)
	{
		Paths.Add(GeoGem::GetShardMesh(Variant).ToSoftObjectPath());
	}

	TestEqual(TEXT("Every tier and shard variant has its own mesh"), Paths.Num(), 4 + GeoGem::ShardVariantCount);
	TestEqual(TEXT("The Cut mesh path"), GeoGem::GetTierMesh(EGeoGemTier::Cut).ToString(),
			  FString(TEXT("/Game/Art/Meshes/Gem/SM_Gem_Cut.SM_Gem_Cut")));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGemDropTest, "GeoTrinity.Gems.Drop", GeoGemTestFlags)

bool FGeoGemDropTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!TestNotNull(TEXT("The catalog is set"), Catalog))
	{
		return false;
	}

	UWorld* World = CreateTestWorld();
	auto const Spawn = [World](FGeoGemDropSpec const& Spec, FVector const& Location)
	{
		FTransform const Transform(Location);
		AGeoGemDrop* Drop = World->SpawnActorDeferred<AGeoGemDrop>(AGeoGemDrop::StaticClass(), Transform);
		Drop->Init(Spec, FVector::ZeroVector);
		Drop->FinishSpawning(Transform);
		return Drop;
	};

	for (EGeoGemTier const Tier : {EGeoGemTier::Chip, EGeoGemTier::Cut, EGeoGemTier::Prism, EGeoGemTier::Core})
	{
		FGeoGemDropSpec Spec;
		Spec.GemId = Catalog->GemsByTier[Tier].Gems[0].Id;
		AGeoGemDrop const* Gem = Spawn(Spec, FVector::ZeroVector);
		UStaticMesh const* const Expected = GeoGem::GetTierMesh(Tier).LoadSynchronous();
		TestNotNull(TEXT("The tier's mesh asset exists (run generate_gem_meshes.py)"), Expected);
		TestTrue(TEXT("A gem drop wears its tier's mesh"), Gem->GetMesh()->GetStaticMesh() == Expected);
	}

	FGeoGemDropSpec Shard;
	Shard.Kind = EGeoGemDropKind::Shard;
	Shard.Variant = 1;
	TestTrue(TEXT("A shard drop wears its variant's mesh"),
			 Spawn(Shard, FVector::ZeroVector)->GetMesh()->GetStaticMesh()
				 == GeoGem::GetShardMesh(1).LoadSynchronous());

	FVector const Rest(0.f, 0.f, 20.f);
	FVector const Air(5000.f, 0.f, 1000.f);
	AGeoGemDrop* Landed = Spawn(Shard, Rest);
	AGeoGemDrop* Flying = Spawn(Shard, Air);
	TestTrue(TEXT("A drop spawned at the rest height is landed"), Landed->IsLanded());
	TestFalse(TEXT("A drop spawned high is still in the air"), Flying->IsLanded());

	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	for (FVector const& Location : {Rest, Air})
	{
		FTransform const Transform(Location);
		APlayableCharacter* const Character =
			World->SpawnActorDeferred<APlayableCharacter>(APlayableCharacter::StaticClass(), Transform);
		Character->SetActorHiddenInGame(true);
		Character->FinishSpawning(Transform);
	}

	World->Tick(LEVELTICK_All, 0.1f);
	TestFalse(TEXT("A landed drop a player character overlaps is destroyed"), IsValid(Landed));
	TestTrue(TEXT("A drop in the air is not picked, character or not"), IsValid(Flying));

	auto const SpawnCounting = [World, &Shard](FVector const& Location)
	{
		FTransform const Transform(Location);
		AGeoTestGemDrop* Drop = World->SpawnActorDeferred<AGeoTestGemDrop>(AGeoTestGemDrop::StaticClass(), Transform);
		Drop->Init(Shard, FVector::ZeroVector);
		Drop->FinishSpawning(Transform);
		return Drop;
	};
	AGeoTestGemDrop* const Thrown = SpawnCounting(FVector(5000.f, 0.f, 1000.f));
	AGeoTestGemDrop* const Lying = SpawnCounting(FVector(9000.f, 0.f, 20.f));
	TestEqual(TEXT("A thrown drop sounds on spawn"), Thrown->SpawnSoundCount, 1);
	TestEqual(TEXT("A drop already lying is silent on spawn"), Lying->SpawnSoundCount, 0);

	Thrown->Destroy();
	TestEqual(TEXT("A drop destroyed in the air is silent"), Thrown->PickupSoundCount, 0);
	Lying->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
	TestEqual(TEXT("A drop ended by anything but a pickup is silent"), Lying->PickupSoundCount, 0);

	AGeoTestGemDrop* const Picked = SpawnCounting(FVector(9000.f, 0.f, 20.f));
	Picked->Destroy();
	TestEqual(TEXT("A landed drop sounds when destroyed"), Picked->PickupSoundCount, 1);

	AGeoGemDrop const* const Falling = Spawn(Shard, Air);
	TestTrue(TEXT("A drop mid-bounce is above its resting place"), Falling->GetBounce(0.1f).Height > 0.f);
	TestTrue(TEXT("A touchdown lights the drop"), Falling->GetBounce(0.f).Flash > 0.9f);
	TestEqual(TEXT("A settled drop no longer bounces"), Falling->GetBounce(30.f).Height, 0.f);
	TestEqual(TEXT("A drop in the air is not settled"), Falling->GetSettled(-1.f), 0.f);
	TestEqual(TEXT("A drop is settled once the bounces are over"), Falling->GetSettled(30.f), 1.f);
	TestEqual(TEXT("A drop pops in from nothing"), Falling->GetPopScale(0.f), 0.f);
	TestEqual(TEXT("A drop pops in to full size"), Falling->GetPopScale(30.f), 1.f);
	TestFalse(TEXT("A flying drop tumbles, never staying upright"),
			  Falling->GetPose(0.5f).GetUpVector().Equals(FVector::UpVector, 0.01f));
	TestTrue(TEXT("A drop's turn winds down to a stop in the pose it reached"),
			 Falling->GetPose(60.f).Equals(Falling->GetPose(90.f), 0.001f));

	auto const SpawnSliding = [World, &Shard](FVector const& Velocity)
	{
		FTransform const Transform(FVector(0.f, 0.f, 20.f));
		AGeoGemDrop* const Drop = World->SpawnActorDeferred<AGeoGemDrop>(AGeoGemDrop::StaticClass(), Transform);
		Drop->Init(Shard, Velocity);
		Drop->FinishSpawning(Transform);
		return Drop;
	};
	AGeoGemDrop const* const Sliding = SpawnSliding(FVector(300.f, 0.f, 0.f));
	TestFalse(TEXT("A drop sliding on the ground is not at rest yet"), Sliding->IsLanded());
	TestTrue(TEXT("Friction stops a sliding drop short of where its speed would carry it"),
			 Sliding->GetRestLocation().X > 100.f && Sliding->GetRestLocation().X < 125.f);

	AStaticMeshActor* const Wall =
		World->SpawnActor<AStaticMeshActor>(FVector(100.f, 0.f, 100.f), FRotator::ZeroRotator);
	Wall->SetMobility(EComponentMobility::Movable);
	Wall->GetStaticMeshComponent()->SetStaticMesh(
		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Wall->SetActorScale3D(FVector(1.f, 20.f, 4.f));
	AGeoGemDrop const* const Walled = SpawnSliding(FVector(1000.f, 0.f, 0.f));
	TestTrue(TEXT("A wall holds a drop in"), Walled->GetRestLocation().X < 50.f);

	AGeoArena const* const Arena = GetDefault<AGeoArena>();
	TestEqual(TEXT("The fountain throws its first drop at once"), Arena->GetFountainThrowTime(0, 5), 0.f);
	TestTrue(TEXT("The fountain throws its last drop after its first"), Arena->GetFountainThrowTime(4, 5) > 0.f);
	TestTrue(TEXT("The fountain bunches its throws at the start"),
			 Arena->GetFountainThrowTime(1, 5) - Arena->GetFountainThrowTime(0, 5)
				 < Arena->GetFountainThrowTime(4, 5) - Arena->GetFountainThrowTime(3, 5));

	DestroyTestWorld(World);
	return true;
}

#endif
