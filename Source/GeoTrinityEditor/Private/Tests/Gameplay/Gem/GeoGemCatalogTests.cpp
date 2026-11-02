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
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestGemDrop.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tests/Gameplay/Gem/GeoGemTestHelpers.h"
#include "Tests/Gameplay/GeoTestFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

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

	TestEqual(TEXT("Every socket has a tier"),
			  CountTier(EGeoGemTier::Chip) + CountTier(EGeoGemTier::Cut) + CountTier(EGeoGemTier::Prism)
				  + CountTier(EGeoGemTier::Core),
			  Sockets.Num());
	TestTrue(TEXT("Some socket is open at level 1"), CountOpenAt(1) > 0);
	TestTrue(TEXT("Sockets open progressively"), CountOpenAt(1) <= CountOpenAt(GeoGem::MaxClassLevel / 2));
	TestEqual(TEXT("Everything is open at the max level"), CountOpenAt(GeoGem::MaxClassLevel), Sockets.Num());
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
	for (EGeoDifficulty const Difficulty :
		 {EGeoDifficulty::Safe, EGeoDifficulty::Reduced, EGeoDifficulty::Original, EGeoDifficulty::Nightmare})
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
	return true;
}

#endif
