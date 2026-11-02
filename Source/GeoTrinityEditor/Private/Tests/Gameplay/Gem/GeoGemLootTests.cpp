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
#include "Tests/Gameplay/GeoTestFlags.h"
#include "Settings/GameDataSettings.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestGemDrop.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tests/Gameplay/Gem/GeoGemTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

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
	bool bNightmareInRange = true;
	for (int32 Roll = 0; Roll < RollCount; ++Roll)
	{
		TMap<EGeoGemTier, int32> const MiniBoss = CountTiers(
			Catalog.RollLoot(Catalog.DropTables.FindChecked(EGeoBossType::MiniBoss), EGeoDifficulty::Original, Stream));
		bMiniBossInRange &= CountAll(MiniBoss) >= 38 && CountAll(MiniBoss) <= 50
			&& !MiniBoss.Contains(EGeoGemTier::Prism) && !MiniBoss.Contains(EGeoGemTier::Core);

		int32 const SafeCount = CountAll(CountTiers(
			Catalog.RollLoot(Catalog.DropTables.FindChecked(EGeoBossType::Boss), EGeoDifficulty::Safe, Stream)));
		bSafeInRange &= SafeCount >= 19 && SafeCount <= 25;

		int32 const NightmareCount = CountAll(CountTiers(
			Catalog.RollLoot(Catalog.DropTables.FindChecked(EGeoBossType::Boss), EGeoDifficulty::Nightmare, Stream)));
		bNightmareInRange &= NightmareCount >= 57 && NightmareCount <= 75;
	}
	TestTrue(TEXT("Mini-boss kill: 38-50 gems, no Prism, no Core"), bMiniBossInRange);
	TestTrue(TEXT("Safe halves the count: 19-25 gems"), bSafeInRange);
	TestTrue(TEXT("Nightmare is half as many again: 57-75 gems"), bNightmareInRange);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------


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

	// Before the drops: their pickup timers go to the game instance's timer manager, the one the world ticks.
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	FVector const Rest(0.f, 0.f, 20.f);
	FVector const Air(5000.f, 0.f, 1000.f);
	AGeoGemDrop* Landed = Spawn(Shard, Rest);
	AGeoGemDrop* Flying = Spawn(Shard, Air);
	TestTrue(TEXT("A drop spawned at the rest height is landed"), Landed->IsLanded());
	TestFalse(TEXT("A drop spawned high is still in the air"), Flying->IsLanded());

	FTransform const CharacterTransform(Rest);
	APlayableCharacter* const Character =
		World->SpawnActorDeferred<APlayableCharacter>(APlayableCharacter::StaticClass(), CharacterTransform);
	Character->SetActorHiddenInGame(true);
	Character->FinishSpawning(CharacterTransform);

	// Short frames, each a new engine frame: the world caps one frame's time (MaxUndilatedFrameTime) and a timer
	// manager ticks once per engine frame.
	auto const TickFor = [World](float const Seconds)
	{
		for (float Ticked = 0.f; Ticked < Seconds; Ticked += 0.1f)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_All, 0.1f);
		}
	};
	TickFor(0.1f);
	TestTrue(TEXT("Before PickupDelay a drop under a player character stays"), IsValid(Landed));
	TickFor(0.5f);
	TestFalse(TEXT("From PickupDelay a drop a player character touches is picked, without moving"), IsValid(Landed));
	TestTrue(TEXT("A drop nobody touches stays"), IsValid(Flying));
	TestTrue(TEXT("A drop still in the air is pickable from PickupDelay"), Flying->IsPickable() && !Flying->IsLanded());

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
	TestEqual(TEXT("A drop destroyed before PickupDelay is silent"), Thrown->PickupSoundCount, 0);
	Lying->RouteEndPlay(EEndPlayReason::RemovedFromWorld);
	TestEqual(TEXT("A drop ended by anything but a pickup is silent"), Lying->PickupSoundCount, 0);

	AGeoTestGemDrop* const Picked = SpawnCounting(FVector(9000.f, 0.f, 20.f));
	TickFor(0.6f);
	Picked->Destroy();
	TestEqual(TEXT("A pickable drop sounds when destroyed"), Picked->PickupSoundCount, 1);

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

	UGameDataSettings* const Settings = GetMutableDefault<UGameDataSettings>();
	TSoftClassPtr<AGeoGemDrop> const SavedDropClass = Settings->GemDropClass;
	Settings->GemDropClass = AGeoTestGemDrop::StaticClass();
	TestEqual(TEXT("An arena with no drop class throws the project one"), Arena->GetGemDropClass().Get(),
			  AGeoTestGemDrop::StaticClass());
	AGeoArena* const OwnClassArena = NewObject<AGeoArena>(GetTransientPackage());
	FClassProperty const* const DropClassProperty =
		CastField<FClassProperty>(AGeoArena::StaticClass()->FindPropertyByName(TEXT("GemDropClass")));
	if (!TestNotNull(TEXT("The arena exposes its GemDropClass property"), DropClassProperty))
	{
		Settings->GemDropClass = SavedDropClass;
		DestroyTestWorld(World);
		return false;
	}
	DropClassProperty->SetPropertyValue_InContainer(OwnClassArena, AGeoGemDrop::StaticClass());
	TestEqual(TEXT("An arena's own drop class wins over the project one"), OwnClassArena->GetGemDropClass().Get(),
			  AGeoGemDrop::StaticClass());
	Settings->GemDropClass = SavedDropClass;

	DestroyTestWorld(World);
	return true;
}

#endif
