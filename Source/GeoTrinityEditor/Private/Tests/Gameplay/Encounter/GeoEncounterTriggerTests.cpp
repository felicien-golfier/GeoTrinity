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
#include "Tests/Gameplay/GeoTestFlags.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/GameDataSettings.h"
#include "StateTree.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestEnemy.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterHealthThresholdTest, "GeoTrinity.Encounter.HealthThreshold",
								 GeoEncounterTestFlags)

bool FGeoEncounterHealthThresholdTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	auto const SetHealth = [](AEnemyCharacter* const Enemy, float const Health)
	{
		GeoASLib::GetGeoAscFromActor(Enemy)->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(),
																	 Health);
	};
	auto const SpawnEnemy = [World, SetHealth](FName const Tag, float const StartHealthRatio)
	{
		AEnemyCharacter* const Enemy = World->SpawnActor<AEnemyCharacter>();
		Enemy->Tags.Add(Tag);
		GeoASLib::GetGeoAscFromActor(Enemy)->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(),
																	 100.f);
		SetHealth(Enemy, 100.f * StartHealthRatio);
		return Enemy;
	};
	auto const StartEncounter =
		[World](FName const Tag, EGeoHealthSide const Side, float const Ratio, UGeoTestAction*& OutReaction)
	{
		AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
		Encounter->AddSteps(1);
		UGeoHealthThresholdTrigger* const Trigger = NewObject<UGeoHealthThresholdTrigger>(Encounter);
		Trigger->Watched = {AEnemyCharacter::StaticClass(), Tag};
		Trigger->Side = Side;
		Trigger->Ratio = Ratio;
		OutReaction = Encounter->NewAction("Reaction");
		Encounter->AddRuleWithTrigger(0, Trigger, {OutReaction}, true);
		Encounter->Start();
	};

	AEnemyCharacter* const Full = SpawnEnemy("Full", 1.f);
	if (TestNotNull(TEXT("The enemy spawns"), Full))
	{
		UGeoTestAction* Below = nullptr;
		StartEncounter("Full", EGeoHealthSide::Below, 0.5f, Below);
		TestEqual(TEXT("Full health is not below half: no firing from OnArm"), Below->Runs, 0);
		SetHealth(Full, 40.f);
		TestEqual(TEXT("Below fires on the crossing"), Below->Runs, 1);
		TestEqual(TEXT("With the ratio as Value"), Below->LastPayload.Value, 0.4f, KINDA_SMALL_NUMBER);
		SetHealth(Full, 30.f);
		TestEqual(TEXT("Not again while staying past"), Below->Runs, 1);
		SetHealth(Full, 80.f);
		SetHealth(Full, 20.f);
		TestEqual(TEXT("Again after leaving and coming back"), Below->Runs, 2);

		UGeoTestAction* Above = nullptr;
		AEnemyCharacter* const Half = SpawnEnemy("Half", 0.5f);
		StartEncounter("Half", EGeoHealthSide::Above, 1.f, Above);
		TestEqual(TEXT("Half health is not above 1.0"), Above->Runs, 0);
		SetHealth(Half, 100.f);
		TestEqual(TEXT("Above 1.0 fires at full health (inclusive)"), Above->Runs, 1);

		UGeoTestAction* AlreadyPast = nullptr;
		StartEncounter("Half", EGeoHealthSide::Above, 0.5f, AlreadyPast);
		TestEqual(TEXT("A state that already holds fires from OnArm"), AlreadyPast->Runs, 1);

		UGeoTestAction* MaxHealthDrop = nullptr;
		AEnemyCharacter* const MostlyFull = SpawnEnemy("Mostly", 0.8f);
		StartEncounter("Mostly", EGeoHealthSide::Above, 1.f, MaxHealthDrop);
		TestEqual(TEXT("80% is not above 1.0"), MaxHealthDrop->Runs, 0);
		GeoASLib::GetGeoAscFromActor(MostlyFull)
			->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), 80.f);
		TestEqual(TEXT("A MaxHealth drop alone that reaches the ratio fires"), MaxHealthDrop->Runs, 1);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterHealthThresholdClassTest, "GeoTrinity.Encounter.HealthThresholdClass",
								 GeoEncounterTestFlags)

bool FGeoEncounterHealthThresholdClassTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoHealthThresholdTrigger* const Trigger = NewObject<UGeoHealthThresholdTrigger>(Encounter);
	Trigger->Watched.Class = AEnemyCharacter::StaticClass();
	Trigger->Side = EGeoHealthSide::Below;
	Trigger->Ratio = 0.3f;
	UGeoTestAction* const Reaction = Encounter->NewAction("Reaction");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction}, true);
	Encounter->Start();

	AEnemyCharacter* const Spawned = World->SpawnActor<AEnemyCharacter>();
	if (TestNotNull(TEXT("The enemy spawns"), Spawned))
	{
		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Spawned);
		ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), 100.f);
		TestEqual(TEXT("A spawn is watched one tick later, not at once"), Reaction->Runs, 0);
		TickTestTimers(*World, 0.f);
		TickTestTimers(*World, 0.1f);
		TestEqual(TEXT("A matching actor spawned after arming fires once watched (ratio 0, below)"), Reaction->Runs, 1);
		TestTrue(TEXT("With the spawned actor as Actor"), Reaction->LastPayload.Actor == Spawned);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActorDiedTest, "GeoTrinity.Encounter.ActorDied", GeoEncounterTestFlags)

bool FGeoEncounterActorDiedTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	AGeoGameState* const GameState = World->SpawnActor<AGeoGameState>();
	World->SetGameState(GameState);

	AEnemyCharacter* const Watched = World->SpawnActor<AEnemyCharacter>();
	AEnemyCharacter* const Other = World->SpawnActor<AEnemyCharacter>();
	AEnemyCharacter* const SecondOfClass = World->SpawnActor<AEnemyCharacter>();
	if (TestTrue(TEXT("The enemies spawn"), Watched && Other && SecondOfClass))
	{
		AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
		Encounter->AddSteps(1);
		Watched->Tags.Add(TEXT("Listed"));
		UGeoActorDiedTrigger* const ListTrigger = NewObject<UGeoActorDiedTrigger>(Encounter);
		ListTrigger->Watched = {AEnemyCharacter::StaticClass(), TEXT("Listed")};
		UGeoTestAction* const ListReaction = Encounter->NewAction("List");
		Encounter->AddRuleWithTrigger(0, ListTrigger, {ListReaction}, true);
		UGeoActorDiedTrigger* const ClassTrigger = NewObject<UGeoActorDiedTrigger>(Encounter);
		ClassTrigger->Watched.Class = AEnemyCharacter::StaticClass();
		UGeoTestAction* const ClassReaction = Encounter->NewAction("Class");
		Encounter->AddRuleWithTrigger(0, ClassTrigger, {ClassReaction}, true);
		Encounter->Start();

		Other->Death();
		TestEqual(TEXT("A character without the filter tag does not fire it"), ListReaction->Runs, 0);
		TestEqual(TEXT("A character of the filter class fires"), ClassReaction->Runs, 1);
		TestEqual(TEXT("Value = watched still alive"), ClassReaction->LastPayload.Value, 2.f);

		Watched->Death();
		TestEqual(TEXT("A character with the filter tag fires it"), ListReaction->Runs, 1);
		TestEqual(TEXT("Count = deaths so far"), ClassReaction->LastPayload.Count, 2);
		TestEqual(TEXT("Value of the list = watched alive"), ListReaction->LastPayload.Value, 0.f);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterPayloadActorDestroyedTest, "GeoTrinity.Encounter.PayloadActorDestroyed",
								 GeoEncounterTestFlags)

bool FGeoEncounterPayloadActorDestroyedTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestLatentAction* const Latent = Encounter->NewAction<UGeoTestLatentAction>("Latent");
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Latent, After});
	Encounter->Start();

	AActor* const Subject = World->SpawnActor<AActor>();
	FGeoEncounterPayload Payload;
	Payload.Actor = Subject;
	Trigger->Fire(Payload);
	Subject->Destroy();
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	Latent->Finish();
	TestEqual(TEXT("The run goes on after the collection"), After->Runs, 1);
	TestNull(TEXT("The destroyed actor reaches the next action as null, never dangling"),
			 After->LastPayload.Actor.Get());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterAbilityActivatedTest, "GeoTrinity.Encounter.AbilityActivated",
								 GeoEncounterTestFlags)

bool FGeoEncounterAbilityActivatedTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestDeployer* const Player = World->SpawnActor<AGeoTestDeployer>();
	PlayerController->SetPawn(Player);
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Player);
	UGeoGameplayAbility const* const Ability = GetDefault<UGeoTestCastAbility>();
	ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestCastAbility::StaticClass(), 1));

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoAbilityActivatedTrigger* const Trigger = NewObject<UGeoAbilityActivatedTrigger>(Encounter);
	Trigger->AbilityTag = UGeoTestCastAbility::GetCastTag();
	UGeoTestAction* const Reaction = Encounter->NewAction("Activated");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction}, true);

	ASC->AbilityActivatedCallbacks.Broadcast(const_cast<UGeoGameplayAbility*>(Ability));
	Encounter->Start();
	TestEqual(TEXT("An activation before arming never counts"), Reaction->Runs, 0);

	ASC->AbilityActivatedCallbacks.Broadcast(const_cast<UGeoGameplayAbility*>(Ability));
	TestEqual(TEXT("The watched ability fires"), Reaction->Runs, 1);
	TestEqual(TEXT("Count is the activations since arming"), Reaction->LastPayload.Count, 1);
	ASC->AbilityActivatedCallbacks.Broadcast(const_cast<UGeoGameplayAbility*>(Ability));
	TestEqual(TEXT("Count goes up"), Reaction->LastPayload.Count, 2);

	Trigger->AbilityTag = FGeoGameplayTags::Get().Ability_Type_Special;
	ASC->AbilityActivatedCallbacks.Broadcast(const_cast<UGeoGameplayAbility*>(Ability));
	TestEqual(TEXT("Another ability's tag does not fire"), Reaction->Runs, 2);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterPlayerIdleTest, "GeoTrinity.Encounter.PlayerIdle", GeoEncounterTestFlags)

bool FGeoEncounterPlayerIdleTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	ADefaultPawn* const Player = World->SpawnActor<ADefaultPawn>();
	PlayerController->SetPawn(Player);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoPlayerIdleTrigger* const Trigger = NewObject<UGeoPlayerIdleTrigger>(Encounter);
	Trigger->Delay = 1.f;
	Trigger->Radius = 50.f;
	UGeoTestAction* const Reaction = Encounter->NewAction("Idle");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction}, true);
	Encounter->Start();
	TickTestTimers(*World, 0.f);

	auto const Poll = [World](int32 const Polls, FVector const Step = FVector::ZeroVector)
	{
		for (int32 Index = 0; Index < Polls; ++Index)
		{
			if (APawn* const Pawn = World->GetFirstPlayerController()->GetPawn())
			{
				Pawn->SetActorLocation(Pawn->GetActorLocation() + Step);
			}
			TickTestTimers(*World, UGeoPlayerIdleTrigger::PollSeconds * 1.01f);
		}
	};
	int32 const PollsPerDelay = FMath::RoundToInt32(Trigger->Delay / UGeoPlayerIdleTrigger::PollSeconds);

	Poll(PollsPerDelay - 1);
	TestEqual(TEXT("Nothing before Delay"), Reaction->Runs, 0);
	Poll(1);
	TestEqual(TEXT("Fires once Delay of stillness passed"), Reaction->Runs, 1);
	TestEqual(TEXT("First tier"), Reaction->LastPayload.Count, 1);
	Poll(PollsPerDelay);
	TestEqual(TEXT("Fires again after another Delay"), Reaction->Runs, 2);
	TestEqual(TEXT("Next tier"), Reaction->LastPayload.Count, 2);

	Poll(PollsPerDelay * 3, FVector(100.f, 0.f, 0.f));
	TestEqual(TEXT("Never while moving"), Reaction->Runs, 2);

	Poll(PollsPerDelay);
	TestEqual(TEXT("Fires once still again"), Reaction->Runs, 3);
	TestEqual(TEXT("A move restarts at the first tier"), Reaction->LastPayload.Count, 1);

	Poll(1, FVector(20.f, 0.f, 0.f));
	Poll(PollsPerDelay - 1);
	TestEqual(TEXT("A drift within Radius stays still"), Reaction->Runs, 4);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterWatchedTagTest, "GeoTrinity.Encounter.WatchedTag", GeoEncounterTestFlags)

bool FGeoEncounterWatchedTagTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoActorDiedTrigger* const DiedTrigger = NewObject<UGeoActorDiedTrigger>(Encounter);
	DiedTrigger->Watched = {AEnemyCharacter::StaticClass(), TEXT("Rude")};
	UGeoTestAction* const DiedReaction = Encounter->NewAction("Died");
	Encounter->AddRuleWithTrigger(0, DiedTrigger, {DiedReaction}, true);
	UGeoHealthThresholdTrigger* const HealthTrigger = NewObject<UGeoHealthThresholdTrigger>(Encounter);
	HealthTrigger->Watched = {AEnemyCharacter::StaticClass(), TEXT("Rude")};
	UGeoTestAction* const HealthReaction = Encounter->NewAction("Health");
	Encounter->AddRuleWithTrigger(0, HealthTrigger, {HealthReaction}, true);
	Encounter->Start();

	AEnemyCharacter* const Tagged = World->SpawnActor<AEnemyCharacter>();
	AEnemyCharacter* const Untagged = World->SpawnActor<AEnemyCharacter>();
	if (TestTrue(TEXT("The enemies spawn"), Tagged && Untagged))
	{
		Tagged->Tags.Add(TEXT("Rude"));
		for (AEnemyCharacter* const Enemy : {Tagged, Untagged})
		{
			GeoASLib::GetGeoAscFromActor(Enemy)->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(),
																		 100.f);
		}

		TickTestTimers(*World, 0.f);
		TickTestTimers(*World, 0.1f);
		TestEqual(TEXT("Only the tagged one fires the health trigger"), HealthReaction->Runs, 1);
		TestTrue(TEXT("It is the tagged one"), HealthReaction->LastPayload.Actor == Tagged);
		Untagged->Death();
		TestEqual(TEXT("An untagged death does not fire"), DiedReaction->Runs, 0);
		Tagged->Death();
		TestEqual(TEXT("A tagged death fires"), DiedReaction->Runs, 1);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterHealthSpawnedSameStepTest, "GeoTrinity.Encounter.HealthSpawnedSameStep",
								 GeoEncounterTestFlags)

bool FGeoEncounterHealthSpawnedSameStepTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	FGameplayTag const PurposeTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>();
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoSpawnEnemiesAction* const Spawner = NewObject<UGeoSpawnEnemiesAction>(Encounter);
	Spawner->EnemyClass = AEnemyCharacter::StaticClass();
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->Setup.Tag = TEXT("Hurt");
	Spawner->Setup.MaxHealth = FScalableFloat(100.f);
	Spawner->Setup.StartHealthRatio = 0.1f;
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner});
	UGeoHealthThresholdTrigger* const HealthTrigger = NewObject<UGeoHealthThresholdTrigger>(Encounter);
	HealthTrigger->Watched = {AEnemyCharacter::StaticClass(), TEXT("Hurt")};
	HealthTrigger->Side = EGeoHealthSide::Above;
	HealthTrigger->Ratio = 1.f;
	UGeoTestAction* const Reaction = Encounter->NewAction("Healed");
	Encounter->AddRuleWithTrigger(0, HealthTrigger, {Reaction}, true);
	Encounter->Start();

	SpawnTrigger->FireNow();
	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 0.1f);
	TActorIterator<AEnemyCharacter> const Found(World);
	if (TestTrue(TEXT("The hurt enemy spawns"), static_cast<bool>(Found)))
	{
		TestEqual(TEXT("Spawned at 10% health, it is not above 1.0"), Reaction->Runs, 0);
		GeoASLib::GetGeoAscFromActor(*Found)->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(),
																	  100.f);
		TestEqual(TEXT("Healed to full, it fires"), Reaction->Runs, 1);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterHealthIgnoresDeadTest, "GeoTrinity.Encounter.HealthIgnoresDead",
								 GeoEncounterTestFlags)

bool FGeoEncounterHealthIgnoresDeadTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	AGeoTestBodyEnemy* const Body = World->SpawnActor<AGeoTestBodyEnemy>();
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Body);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 100.f);
	Body->Death();

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoHealthThresholdTrigger* const Trigger = NewObject<UGeoHealthThresholdTrigger>(Encounter);
	Trigger->Watched.Class = AEnemyCharacter::StaticClass();
	Trigger->Side = EGeoHealthSide::Above;
	Trigger->Ratio = 1.f;
	UGeoTestAction* const Reaction = Encounter->NewAction("Reaction");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction});
	Encounter->Start();
	TestEqual(TEXT("A dead body at full health is on no side"), Reaction->Runs, 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterHealthFiredOnArmTest, "GeoTrinity.Encounter.HealthFiredOnArm",
								 GeoEncounterTestFlags)

bool FGeoEncounterHealthFiredOnArmTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	auto const SpawnFull = [World]
	{
		AEnemyCharacter* const Enemy = World->SpawnActor<AEnemyCharacter>();
		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Enemy);
		ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), 100.f);
		ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 100.f);
		return Enemy;
	};

	TArray<AEnemyCharacter*> const Full = {SpawnFull(), SpawnFull()};
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoHealthThresholdTrigger* const Trigger = NewObject<UGeoHealthThresholdTrigger>(Encounter);
	Trigger->Watched.Class = AEnemyCharacter::StaticClass();
	Trigger->Side = EGeoHealthSide::Above;
	Trigger->Ratio = 1.f;
	UGeoTestAction* const Reaction = Encounter->NewAction("Reaction");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction});
	Encounter->Start();
	TestEqual(TEXT("Already full: fires once from OnArm"), Reaction->Runs, 1);

	AEnemyCharacter* const Late = SpawnFull();
	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 0.1f);
	for (AEnemyCharacter* const Enemy : {Full[0], Full[1], Late})
	{
		TestFalse(TEXT("Disarmed by its own firing: nobody stays watched, a later spawn neither"),
				  GeoASLib::GetGeoAscFromActor(Enemy)
					  ->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute())
					  .IsBoundToObject(Trigger));
	}

	DestroyTestWorld(World);
	return true;
}

#endif
