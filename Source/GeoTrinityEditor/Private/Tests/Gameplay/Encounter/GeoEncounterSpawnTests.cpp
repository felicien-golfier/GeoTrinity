// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Buttons/GeoTeleporter.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Actor/GeoCat.h"
#include "Actor/GeoTargetPoint.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/GeoEnemySetup.h"
#include "Characters/PlayableCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Encounter/Action/Cat/GeoCatMontageAction.h"
#include "Encounter/Action/Cat/GeoCatRoamAction.h"
#include "Encounter/Action/Cat/GeoCatWalkAction.h"
#include "Encounter/Action/GeoActivateAbilityAction.h"
#include "Encounter/Action/GeoAddToCounterAction.h"
#include "Encounter/Action/GeoCatLineAction.h"
#include "Encounter/Action/GeoCatLineFromFileAction.h"
#include "Encounter/Action/GeoDelayAction.h"
#include "Encounter/Action/GeoQuestCardAction.h"
#include "Encounter/Action/GeoSetActorHiddenAction.h"
#include "Encounter/Action/GeoSetMovementLockedAction.h"
#include "Encounter/Action/GeoSetTeleporterOpenAction.h"
#include "Encounter/Action/GeoShowAbilityPageAction.h"
#include "Encounter/Action/GeoSpawnEnemiesAction.h"
#include "Encounter/Action/GeoSpawnZoneAction.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Encounter/Trigger/GeoAbilityActivatedTrigger.h"
#include "Encounter/Trigger/GeoActorDiedTrigger.h"
#include "Encounter/Trigger/GeoCounterTrigger.h"
#include "Encounter/Trigger/GeoEventTrigger.h"
#include "Encounter/Trigger/GeoHealthThresholdTrigger.h"
#include "Encounter/Trigger/GeoPlayerIdleTrigger.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameplayTagsManager.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/GameDataSettings.h"
#include "StateTree.h"
#include "Tests/Gameplay/Encounter/GeoEncounterTestHelpers.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestEnemy.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoEncounterTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSpawnEnemiesTest, "GeoTrinity.Encounter.SpawnEnemies",
								 GeoEncounterTestFlags)

bool FGeoEncounterSpawnEnemiesTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	FGameplayTag const PurposeTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	for (int32 Index = 0; Index < 2; ++Index)
	{
		AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(Index * 300.f, 0.f, 0.f), FRotator());
		Point->GameplayTags.AddTag(PurposeTag);
		Point->GameplayTags.AddTag(ArenaTag);
	}

	auto const Living = [World]
	{
		TArray<AEnemyCharacter*> Enemies;
		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			if (!It->IsDead())
			{
				Enemies.Add(*It);
			}
		}

		return Enemies;
	};

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoSpawnEnemiesAction* const Spawner = NewObject<UGeoSpawnEnemiesAction>(Encounter);
	Spawner->EnemyClass = AEnemyCharacter::StaticClass();
	Spawner->Count = 3;
	Spawner->RespawnDelay = 2.f;
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->bKillOnStepExit = true;
	Spawner->Setup.Team = ETeam::Player;
	Spawner->Setup.Tag = TEXT("Rude");
	Spawner->Setup.MaxHealth = FScalableFloat(200.f);
	Spawner->Setup.StartHealthRatio = 0.5f;
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	SpawnTrigger->FireNow();
	TickTestTimers(*World, 0.f);
	TArray<AEnemyCharacter*> const First = Living();
	TestEqual(TEXT("Count enemies spawn"), First.Num(), 3);
	if (First.Num() == 3)
	{
		UGeoAbilitySystemComponent* const FirstASC = GeoASLib::GetGeoAscFromActor(First[0]);
		TestEqual(TEXT("The setup's team"), First[0]->GetGenericTeamId(),
				  FGenericTeamId(static_cast<uint8>(ETeam::Player)));
		TestTrue(TEXT("The setup's tag"), First[0]->ActorHasTag(TEXT("Rude")));
		TestEqual(TEXT("The setup's max health"),
				  FirstASC->GetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute()), 200.f);
		TestEqual(TEXT("The setup's start health"),
				  FirstASC->GetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute()), 100.f);
		FirstASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 0.f);
		TestTrue(TEXT("With bResetToFullLife false it dies at zero"), First[0]->IsDead());
		TestEqual(TEXT("A death leaves two"), Living().Num(), 2);
		TickTestTimers(*World, 1.01f);
		TestEqual(TEXT("Not refilled before RespawnDelay"), Living().Num(), 2);
		TickTestTimers(*World, 1.01f);
		TestEqual(TEXT("Refilled after RespawnDelay"), Living().Num(), 3);
	}

	LeaveTrigger->FireNow();
	TestEqual(TEXT("Leaving the step kills them all"), Living().Num(), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSpawnEnemiesRespawnTest, "GeoTrinity.Encounter.SpawnEnemiesRespawn",
								 GeoEncounterTestFlags)

bool FGeoEncounterSpawnEnemiesRespawnTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	FGameplayTag const PurposeTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(500.f, 0.f, 0.f), FRotator());
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoSpawnEnemiesAction* const Spawner = NewObject<UGeoSpawnEnemiesAction>(Encounter);
	Spawner->EnemyClass = AEnemyCharacter::StaticClass();
	Spawner->Count = 2;
	Spawner->RespawnDelay = 5.f;
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();
	SpawnTrigger->FireNow();

	auto const Living = [World]
	{
		TArray<AEnemyCharacter*> Enemies;
		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			if (!It->IsDead())
			{
				Enemies.Add(*It);
			}
		}

		return Enemies;
	};

	TArray<AEnemyCharacter*> const First = Living();
	if (TestEqual(TEXT("Count enemies spawn"), First.Num(), 2))
	{
		FVector const DeathLocation(100.f, 200.f, 0.f);
		First[0]->SetActorLocation(DeathLocation);
		First[0]->Death();
		TickTestTimers(*World, 0.f);
		TickTestTimers(*World, 3.f);
		First[1]->Death();
		TickTestTimers(*World, 0.f);
		TickTestTimers(*World, 2.1f);
		TArray<AEnemyCharacter*> const AfterFirstTimer = Living();
		TestEqual(TEXT("Only the first one is back, the second waits for its own timer"), AfterFirstTimer.Num(), 1);
		if (AfterFirstTimer.Num() == 1)
		{
			TestEqual(TEXT("It comes back where it died"), AfterFirstTimer[0]->GetActorLocation().X, DeathLocation.X);
		}

		TickTestTimers(*World, 3.f);
		TestEqual(TEXT("The second comes back 5 s after its own death"), Living().Num(), 2);

		Living()[0]->Death();
		LeaveTrigger->FireNow();
		TickTestTimers(*World, 0.f);
		TickTestTimers(*World, 6.f);
		TestEqual(TEXT("Leaving the step drops the pending respawn; the living one stays (no bKillOnStepExit)"),
				  Living().Num(), 1);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSpawnEnemiesHeldTest, "GeoTrinity.Encounter.SpawnEnemiesHeld",
								 GeoEncounterTestFlags)

bool FGeoEncounterSpawnEnemiesHeldTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	FGameplayTag const PurposeTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(500.f, 0.f, 0.f), FRotator());
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	auto const Living = [World]
	{
		int32 Enemies = 0;
		for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
		{
			Enemies += It->IsDead() ? 0 : 1;
		}

		return Enemies;
	};

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoSpawnEnemiesAction* const Spawner = NewObject<UGeoSpawnEnemiesAction>(Encounter);
	Spawner->EnemyClass = AEnemyCharacter::StaticClass();
	Spawner->RespawnDelay = 2.f;
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->bKillOnStepExit = true;
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner, After}, true);
	Encounter->Start();

	SpawnTrigger->FireNow();
	TestEqual(TEXT("The enemy spawns"), Living(), 1);
	TestEqual(TEXT("The action after the spawner runs at once"), After->Runs, 1);

	SpawnTrigger->FireNow();
	TestEqual(TEXT("Run again: the previous run's enemy is killed, one new one lives"), Living(), 1);

	for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
	{
		if (!It->IsDead())
		{
			It->Death();
		}
	}

	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 2.1f);
	TestEqual(TEXT("One respawn per death, not one per run"), Living(), 1);

	Encounter->Restart();
	TestEqual(TEXT("Restart kills the held spawner's enemy"), Living(), 0);
	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 3.f);
	TestEqual(TEXT("No respawn once the step is left"), Living(), 0);

	SpawnTrigger->FireNow();
	Encounter->Abort();
	TestEqual(TEXT("Abort kills the held spawner's enemy"), Living(), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSpawnZoneHeldTest, "GeoTrinity.Encounter.SpawnZoneHeld",
								 GeoEncounterTestFlags)

bool FGeoEncounterSpawnZoneHeldTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoEffectZone::StaticClass();

	FGameplayTag const PurposeTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("TargetPoint.ZoneSpawn"));
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(400.f, 0.f, 0.f), FRotator());
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoSpawnZoneAction* const Spawner = NewObject<UGeoSpawnZoneAction>(Encounter);
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->Params.Size = 300.f;
	Spawner->Params.Color.Color = EGeoColor::Heal;
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner, After}, true);
	Encounter->Start();

	SpawnTrigger->FireNow();
	TestEqual(TEXT("The action after the zone runs at once"), After->Runs, 1);
	TActorIterator<AGeoEffectZone> const FirstFound(World);
	if (TestTrue(TEXT("The zone spawns"), static_cast<bool>(FirstFound)))
	{
		AGeoEffectZone const* const First = *FirstFound;
		SpawnTrigger->FireNow();
		AGeoEffectZone const* Second = nullptr;
		for (TActorIterator<AGeoEffectZone> It(World); It; ++It)
		{
			Second = *It != First ? *It : Second;
		}

		TestFalse(TEXT("Run again: the previous zone is recalled"), First->IsActive());
		if (TestNotNull(TEXT("Run again: a new zone spawns"), Second))
		{
			TestTrue(TEXT("The new zone is up"), Second->IsActive());
			Encounter->Abort();
			TestFalse(TEXT("Abort recalls the held zone"), Second->IsActive());
		}
	}

	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoArenaSpawnAddResetTest, "GeoTrinity.Arena.SpawnAddReset", GeoEncounterTestFlags)

bool FGeoArenaSpawnAddResetTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	FGeoEnemySetup Setup;
	Setup.Tag = TEXT("Ally");
	AEnemyCharacter* const Add =
		Arena->SpawnAdd(AEnemyCharacter::StaticClass(), FTransform(FVector(300.f, 0.f, 999.f)), &Setup);
	if (TestNotNull(TEXT("SpawnAdd spawns"), Add))
	{
		TestTrue(TEXT("The setup is applied"), Add->ActorHasTag(TEXT("Ally")));
		TestTrue(TEXT("The add knows its arena"), Add->Arena.Get() == Arena);
		TestEqual(TEXT("At character height"), Add->GetActorLocation().Z, static_cast<double>(ArbitraryCharacterZ));
		Arena->ResetBoss();
		TestFalse(TEXT("A wipe reset removes it"), IsValid(Add));
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSpawnZoneTest, "GeoTrinity.Encounter.SpawnZone", GeoEncounterTestFlags)

bool FGeoEncounterSpawnZoneTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoEffectZone::StaticClass();

	FGameplayTag const PurposeTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("TargetPoint.ZoneSpawn"));
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(400.f, 0.f, 0.f), FRotator());
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoSpawnZoneAction* const Spawner = NewObject<UGeoSpawnZoneAction>(Encounter);
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->Params.Size = 300.f;
	Spawner->Params.Color.Color = EGeoColor::Damage;
	Spawner->Team = ETeam::Enemy;
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	SpawnTrigger->FireNow();
	TActorIterator<AGeoEffectZone> const Found(World);
	if (TestTrue(TEXT("The zone spawns"), static_cast<bool>(Found)))
	{
		AGeoEffectZone const* const Zone = *Found;
		TestEqual(TEXT("On the tagged point"), Zone->GetActorLocation().X, 400.0);
		TestEqual(TEXT("With the action's team"), Zone->GetGenericTeamId(),
				  FGenericTeamId(static_cast<uint8>(ETeam::Enemy)));
		LeaveTrigger->FireNow();
		TestFalse(TEXT("Leaving the step ends it"), Zone->IsActive());
	}

	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

#endif
