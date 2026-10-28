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
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestEnemy.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoEncounterTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

using FEncounterActions = TArray<TObjectPtr<UGeoEncounterAction>>;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterStepFlowTest, "GeoTrinity.Encounter.StepFlow", GeoEncounterTestFlags)

bool FGeoEncounterStepFlowTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	TArray<FName> Order;
	Encounter->AddSteps(2);
	Encounter->EditSteps()[0].OnEnter = {Encounter->NewAction("EnterA", &Order)};
	Encounter->EditSteps()[0].OnExit = {Encounter->NewAction("ExitA", &Order)};
	Encounter->EditSteps()[1].OnEnter = {Encounter->NewAction("EnterB", &Order)};
	UGeoTestTrigger* const TriggerA = Encounter->AddRule(
		0, {Encounter->NewAction("First", &Order), Encounter->NewAction("Second", &Order)}, false, true);
	UGeoTestTrigger* const TriggerB = Encounter->AddRule(1, {});

	Encounter->Start();
	TestEqual(TEXT("Entering A arms its trigger"), TriggerA->ArmCount, 1);
	TestEqual(TEXT("B is not armed yet"), TriggerB->ArmCount, 0);
	TestTrue(TEXT("OnEnter runs alongside the armed rules"), Order == TArray<FName>{"EnterA"});

	TriggerA->FireNow();
	TestTrue(TEXT("Actions run in order, then OnExit, then the next step's OnEnter"),
			 Order == TArray<FName>{"EnterA", "First", "Second", "ExitA", "EnterB"});
	TestEqual(TEXT("B is armed"), TriggerB->ArmCount, 1);
	TestEqual(TEXT("A's trigger was disarmed once"), TriggerA->DisarmCount, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterConditionTest, "GeoTrinity.Encounter.Condition", GeoEncounterTestFlags)

bool FGeoEncounterConditionTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const Equals = Encounter->NewAction("Equals");
	UGeoTestAction* const Multiple = Encounter->NewAction("Multiple");
	UGeoTestTrigger* const EqualsTrigger =
		Encounter->AddRule(0, {Equals}, true, false, NAME_None,
						   Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 2.f));
	UGeoTestTrigger* const MultipleTrigger = Encounter->AddRule(
		0, {Multiple}, true, false, NAME_None,
		Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::MultipleOf, 3.f));
	Encounter->Start();

	for (int32 Count = 1; Count <= 3; ++Count)
	{
		EqualsTrigger->FireNow(Count);
	}
	TestEqual(TEXT("Count == 2 runs the rule on the second firing only"), Equals->Runs, 1);

	for (int32 Count = 1; Count <= 6; ++Count)
	{
		MultipleTrigger->FireNow(Count);
	}
	TestEqual(TEXT("MultipleOf 3 passes on 3 and 6"), Multiple->Runs, 2);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterCounterTest, "GeoTrinity.Encounter.Counter", GeoEncounterTestFlags)

bool FGeoEncounterCounterTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);

	UGeoAddToCounterAction* const Add = NewObject<UGeoAddToCounterAction>(Encounter);
	Add->CounterName = "X";
	UGeoTestAction* const OnWatched = Encounter->NewAction("OnWatched");
	UGeoTestAction* const OnCompare = Encounter->NewAction("OnCompare");

	UGeoTestTrigger* const AddTrigger = Encounter->AddRule(0, {Add}, true);
	UGeoCounterTrigger* const CounterTrigger = NewObject<UGeoCounterTrigger>(Encounter);
	CounterTrigger->CounterName = "X";
	Encounter->AddRuleWithTrigger(
		0, CounterTrigger, {OnWatched}, true,
		Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 2.f));
	UGeoTestTrigger* const CompareTrigger =
		Encounter->AddRule(0, {OnCompare}, true, false, NAME_None,
						   Encounter->NewCompare(EGeoCompareSource::Counter, EGeoCompareOperator::Equal, 2.f, "X"));
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	AddTrigger->FireNow();
	CompareTrigger->FireNow();
	TestEqual(TEXT("The counter is 1: the compare on 2 fails"), OnCompare->Runs, 0);
	TestEqual(TEXT("The counter trigger fired with 1: its condition fails"), OnWatched->Runs, 0);

	AddTrigger->FireNow();
	CompareTrigger->FireNow();
	TestEqual(TEXT("The counter is 2: the compare passes"), OnCompare->Runs, 1);
	TestEqual(TEXT("The counter trigger fired with the new value"), OnWatched->Runs, 1);
	TestEqual(TEXT("GetCounter reads it"), Encounter->GetCounter("X"), 2);

	LeaveTrigger->FireNow();
	TestEqual(TEXT("Counters are at 0 on step entry"), Encounter->GetCounter("X"), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterRepeatTest, "GeoTrinity.Encounter.Repeat", GeoEncounterTestFlags)

bool FGeoEncounterRepeatTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const Once = Encounter->NewAction("Once");
	UGeoTestAction* const Every = Encounter->NewAction("Every");
	UGeoTestTrigger* const OnceTrigger = Encounter->AddRule(0, {Once}, false);
	UGeoTestTrigger* const EveryTrigger = Encounter->AddRule(0, {Every}, true);
	Encounter->Start();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		OnceTrigger->FireNow();
		EveryTrigger->FireNow();
	}
	TestEqual(TEXT("bRepeat false fires once"), Once->Runs, 1);
	TestEqual(TEXT("bRepeat false disarms the trigger"), OnceTrigger->DisarmCount, 1);
	TestEqual(TEXT("bRepeat true fires every passing firing"), Every->Runs, 3);
	TestEqual(TEXT("bRepeat true stays armed"), EveryTrigger->DisarmCount, 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterLatentActionTest, "GeoTrinity.Encounter.LatentAction",
								 GeoEncounterTestFlags)

bool FGeoEncounterLatentActionTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTestLatentAction* const Latent = Encounter->NewAction<UGeoTestLatentAction>("Latent");
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestLatentAction* const Pending = Encounter->NewAction<UGeoTestLatentAction>("Pending");
	UGeoTestAction* const OnEnterB = Encounter->NewAction("EnterB");
	Encounter->EditSteps()[1].OnEnter = {OnEnterB};
	UGeoTestTrigger* const WaitTrigger = Encounter->AddRule(0, {Latent, After});
	UGeoTestTrigger* const PendingTrigger = Encounter->AddRule(0, {Pending});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	UGeoTestTrigger* const TriggerB = Encounter->AddRule(1, {});
	Encounter->Start();

	WaitTrigger->FireNow();
	TestEqual(TEXT("The latent action ran"), Latent->Runs, 1);
	TestEqual(TEXT("The next action waits"), After->Runs, 0);
	Latent->Finish();
	TestEqual(TEXT("Finish lets the next action run"), After->Runs, 1);

	PendingTrigger->FireNow();
	LeaveTrigger->FireNow();
	TestEqual(TEXT("Leaving the step cancels the running latent action"), Pending->Cancels, 1);
	Pending->Finish();
	TestEqual(TEXT("A late Finish changes nothing"), TriggerB->ArmCount, 1);
	TestEqual(TEXT("B's OnEnter ran once"), OnEnterB->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterBusyRuleTest, "GeoTrinity.Encounter.BusyRule", GeoEncounterTestFlags)

bool FGeoEncounterBusyRuleTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestLatentAction* const Latent = Encounter->NewAction<UGeoTestLatentAction>("Latent");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Latent}, true);
	Encounter->Start();

	Trigger->FireNow();
	Trigger->FireNow();
	TestEqual(TEXT("A firing while the actions run is ignored"), Latent->Runs, 1);
	Latent->Finish();
	Trigger->FireNow();
	TestEqual(TEXT("Once they finished the rule fires again"), Latent->Runs, 2);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterStepAfterTest, "GeoTrinity.Encounter.StepAfter", GeoEncounterTestFlags)

bool FGeoEncounterStepAfterTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	UGeoTestCompletionListener* const Listener = NewObject<UGeoTestCompletionListener>(Encounter);
	Encounter->OnCompleted.AddDynamic(Listener, &UGeoTestCompletionListener::OnCompleted);
	Encounter->AddSteps(3);

	UGeoTestTrigger* const JumpToC = Encounter->AddRule(0, {}, false, true, "C");
	UGeoTestTrigger* const TriggerB = Encounter->AddRule(1, {});
	UGeoTestGoToStepAction* const GoToSelf = Encounter->NewAction<UGeoTestGoToStepAction>("GoToSelf");
	GoToSelf->StepName = "B";
	UGeoTestTrigger* const ReenterB = Encounter->AddRule(1, {GoToSelf});
	UGeoTestTrigger* const ToC = Encounter->AddRule(1, {}, false, true);
	UGeoTestGoToStepAction* const GoToB = Encounter->NewAction<UGeoTestGoToStepAction>("GoToB");
	GoToB->StepName = "B";
	UGeoTestTrigger* const BackToB = Encounter->AddRule(2, {GoToB});
	UGeoTestTrigger* const Last = Encounter->AddRule(2, {}, false, true);
	Encounter->Start();

	JumpToC->FireNow();
	TestEqual(TEXT("StepAfter jumps over B"), TriggerB->ArmCount, 0);

	BackToB->FireNow();
	TestEqual(TEXT("GoToStep enters the named step"), TriggerB->ArmCount, 1);

	ReenterB->FireNow();
	TestEqual(TEXT("GoToStep of the current step re-enters it"), TriggerB->ArmCount, 2);
	TestEqual(TEXT("The first visit's triggers were disarmed"), TriggerB->DisarmCount, 1);

	ToC->FireNow();
	TestEqual(TEXT("Not completed while a step follows"), Listener->Completions, 0);
	Last->FireNow();
	TestEqual(TEXT("Completing the last step raises OnCompleted"), Listener->Completions, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterReentrantTest, "GeoTrinity.Encounter.Reentrant", GeoEncounterTestFlags)

bool FGeoEncounterReentrantTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(3);
	TArray<FName> Order;

	UGeoTestGoToStepAction* const GoToB = Encounter->NewAction<UGeoTestGoToStepAction>("GoToB", &Order);
	GoToB->StepName = "B";
	UGeoTestAction* const AfterGoTo = Encounter->NewAction("AfterGoTo", &Order);
	Encounter->EditSteps()[0].OnExit = {Encounter->NewAction("ExitA", &Order)};
	UGeoTestTrigger* const TriggerA = Encounter->AddRule(0, {GoToB, AfterGoTo});

	UGeoTestAction* const EnterB = Encounter->NewAction("EnterB", &Order);
	Encounter->EditSteps()[1].OnEnter = {EnterB};
	UGeoTestTrigger* const SelfCompleting = NewObject<UGeoTestTrigger>(Encounter);
	SelfCompleting->bFireOnArm = true;
	Encounter->AddRuleWithTrigger(1, SelfCompleting, {});
	Encounter->EditSteps()[1].Rules[0].bCompletesStep = true;
	UGeoTestTrigger* const TriggerC = Encounter->AddRule(2, {});
	Encounter->Start();

	TriggerA->FireNow();
	TestTrue(TEXT("One clean change: the request, A left once, nothing else on A or B"),
			 Order == TArray<FName>{"GoToB", "ExitA"});
	TestEqual(TEXT("No action ran on the left step after the request"), AfterGoTo->Runs, 0);
	TestEqual(TEXT("B's trigger fired from OnArm and completed B before its OnEnter"), EnterB->Runs, 0);
	TestEqual(TEXT("C is entered once"), TriggerC->ArmCount, 1);
	TestEqual(TEXT("B's trigger was armed once and disarmed once"),
			  SelfCompleting->ArmCount + SelfCompleting->DisarmCount, 2);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterRestartTest, "GeoTrinity.Encounter.Restart", GeoEncounterTestFlags)

bool FGeoEncounterRestartTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTestTrigger* const TriggerA = Encounter->AddRule(0, {}, false, true);
	UGeoTestTrigger* const TriggerB = Encounter->AddRule(1, {});
	Encounter->Start();
	TriggerA->FireNow();
	Encounter->AddToEncounterCounter("X", 3);
	TestEqual(TEXT("B is the current step"), TriggerB->ArmCount, 1);

	Encounter->Restart();
	TestEqual(TEXT("B's trigger is disarmed"), TriggerB->DisarmCount, 1);
	TestEqual(TEXT("A is entered again"), TriggerA->ArmCount, 2);
	TestEqual(TEXT("Counters are at 0"), Encounter->GetCounter("X"), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterEventBusTest, "GeoTrinity.Encounter.EventBus", GeoEncounterTestFlags)

bool FGeoEncounterEventBusTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	FGameplayTag const RootTag = FGameplayTag::RequestGameplayTag("Encounter.Event");
	UGeoEventTrigger* const Trigger = NewObject<UGeoEventTrigger>(Encounter);
	Trigger->Tag = RootTag;
	UGeoTestAction* const Reaction = Encounter->NewAction("Reaction");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction}, true);
	Encounter->Start();

	FGeoEncounterPayload Payload;
	Payload.Count = 7;
	World->GetSubsystem<UGeoEncounterSubsystem>()->Raise(RootTag, Payload);
	TestEqual(TEXT("The raised event reaches the trigger"), Reaction->Runs, 1);
	TestEqual(TEXT("With its payload"), Reaction->LastPayload.Count, 7);
	TestTrue(TEXT("And its tag"), Reaction->LastPayload.Tag == RootTag);

	DestroyTestWorld(World);
	return true;
}

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterRestartWhileExitingTest, "GeoTrinity.Encounter.RestartWhileExiting",
								 GeoEncounterTestFlags)

bool FGeoEncounterRestartWhileExitingTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTestLatentAction* const Exit = Encounter->NewAction<UGeoTestLatentAction>("Exit");
	Encounter->EditSteps()[0].OnExit = {Exit};
	UGeoTestTrigger* const TriggerA = Encounter->AddRule(0, {}, false, true);
	UGeoTestTrigger* const TriggerB = Encounter->AddRule(1, {});
	Encounter->Start();

	TriggerA->FireNow();
	TestEqual(TEXT("A's OnExit is running"), Exit->Runs, 1);
	Encounter->Restart();
	TestEqual(TEXT("Restart cancels the running OnExit"), Exit->Cancels, 1);
	TestEqual(TEXT("A is entered again"), TriggerA->ArmCount, 2);
	Exit->Finish();
	TestEqual(TEXT("The cancelled OnExit never leads to B"), TriggerB->ArmCount, 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterStartWhileRunningTest, "GeoTrinity.Encounter.StartWhileRunning",
								 GeoEncounterTestFlags)

bool FGeoEncounterStartWhileRunningTest::RunTest(FString const& /*Parameters*/)
{
	AddExpectedMessage(TEXT("Start while running"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains,
					   1, false);
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();
	Encounter->Start();
	TestEqual(TEXT("A second Start is refused"), Trigger->ArmCount, 1);

	Trigger->FireNow();
	Encounter->Start();
	TestEqual(TEXT("A completed encounter starts again"), Trigger->ArmCount, 2);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterServerOnlyTest, "GeoTrinity.Encounter.ServerOnly", GeoEncounterTestFlags)

bool FGeoEncounterServerOnlyTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {});

	World->SetPlayInEditorInitialNetMode(NM_Client);
	Encounter->Start();
	Encounter->Restart();
	TestEqual(TEXT("A client runs nothing"), Trigger->ArmCount, 0);

	World->SetPlayInEditorInitialNetMode(NM_ListenServer);
	Encounter->Start();
	TestEqual(TEXT("The server runs it"), Trigger->ArmCount, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterEndPlayTest, "GeoTrinity.Encounter.EndPlay", GeoEncounterTestFlags)

bool FGeoEncounterEndPlayTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestLatentAction* const Latent = Encounter->NewAction<UGeoTestLatentAction>("Latent");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Latent}, true);
	Encounter->Start();
	Trigger->FireNow();

	Encounter->Destroy();
	TestEqual(TEXT("Ending play disarms the step's triggers"), Trigger->DisarmCount, 1);
	TestEqual(TEXT("Ending play cancels the running action"), Latent->Cancels, 1);
	Encounter->Restart();
	TestEqual(TEXT("No step change once play ended"), Trigger->ArmCount, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterValidationTest, "GeoTrinity.Encounter.Validation", GeoEncounterTestFlags)

bool FGeoEncounterValidationTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	auto const ErrorsOf = [World](TFunctionRef<void(AGeoTestEncounter&)> Build)
	{
		AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
		Build(*Encounter);
		FDataValidationContext Context;
		Encounter->IsDataValid(Context);
		return Context.GetNumErrors();
	};

	TestEqual(TEXT("A sound encounter has no error"),
			  ErrorsOf(
				  [](AGeoTestEncounter& Encounter)
				  {
					  Encounter.AddSteps(2);
					  Encounter.AddRule(0, {}, false, true, "B");
				  }),
			  0u);
	TestTrue(TEXT("A duplicate step name is flagged"),
			 ErrorsOf(
				 [](AGeoTestEncounter& Encounter)
				 {
					 Encounter.AddSteps(1);
					 Encounter.AddSteps(1);
				 })
				 > 0u);
	TestTrue(TEXT("An action after a start action is flagged"),
			 ErrorsOf(
				 [](AGeoTestEncounter& Encounter)
				 {
					 Encounter.AddSteps(1);
					 Encounter.AddRule(0, {Encounter.NewStartAction(&Encounter), Encounter.NewAction("After")});
				 })
				 > 0u);
	TestEqual(TEXT("A start action last in its list is fine"),
			  ErrorsOf(
				  [](AGeoTestEncounter& Encounter)
				  {
					  Encounter.AddSteps(1);
					  Encounter.AddRule(0, {Encounter.NewAction("Before"), Encounter.NewStartAction(&Encounter)});
				  }),
			  0u);
	TestTrue(TEXT("An unknown StepAfter is flagged"),
			 ErrorsOf(
				 [](AGeoTestEncounter& Encounter)
				 {
					 Encounter.AddSteps(1);
					 Encounter.AddRule(0, {}, false, true, "Nowhere");
				 })
				 > 0u);
	TestTrue(TEXT("A null trigger is flagged"),
			 ErrorsOf(
				 [](AGeoTestEncounter& Encounter)
				 {
					 Encounter.AddSteps(1);
					 Encounter.AddRuleWithTrigger(0, nullptr, {});
					 Encounter.EditSteps()[0].Rules[0].bCompletesStep = true;
				 })
				 > 0u);
	TestTrue(TEXT("A counter compare without a name is flagged"),
			 ErrorsOf(
				 [](AGeoTestEncounter& Encounter)
				 {
					 Encounter.AddSteps(1);
					 Encounter.AddRule(
						 0, {}, false, true, NAME_None,
						 Encounter.NewCompare(EGeoCompareSource::Counter, EGeoCompareOperator::Equal, 1.f));
				 })
				 > 0u);

	DestroyTestWorld(World);
	return true;
}

/** A game instance and a game state in World, so an enemy can spawn. */
static void PrepareEnemyWorld(FAutomationTestBase& Test, UWorld& World)
{
	World.SetGameInstance(NewObject<UGameInstance>(GEngine));
	World.SetGameState(World.SpawnActor<AGeoGameState>());
}

/** A caster arena in World, its hidden caster spawned. The adds it spawns are test enemies with no DefaultAttributes.
 */
static AGeoTestCasterArena* SpawnCasterArena(FAutomationTestBase& Test, UWorld& World)
{
	PrepareEnemyWorld(Test, World);
	Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	return World.SpawnActor<AGeoTestCasterArena>();
}

/** The ASC of World's shared caster, AbilityClass granted to it; Handle is its spec. */
static UGeoAbilitySystemComponent* GrantShared(UWorld& World, TSubclassOf<UGeoGameplayAbility> const AbilityClass,
											   FGameplayAbilitySpecHandle& Handle)
{
	UGeoEncounterSubsystem* const Subsystem = World.GetSubsystem<UGeoEncounterSubsystem>();
	Handle = Subsystem->GrantSharedAbility(AbilityClass);
	return GeoASLib::GetGeoAscFromActor(Subsystem->GetSharedCaster());
}

static UGeoActivateAbilityAction* NewActivateAction(AGeoTestEncounter& Encounter,
													TSubclassOf<UGeoGameplayAbility> const AbilityClass,
													bool const bWaitForEnd = true)
{
	UGeoActivateAbilityAction* const Action = NewObject<UGeoActivateAbilityAction>(&Encounter);
	Action->AbilityClass = AbilityClass;
	Action->bWaitForEnd = bWaitForEnd;
	return Action;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSharedCasterTest, "GeoTrinity.Encounter.SharedCaster",
								 GeoEncounterTestFlags)

bool FGeoEncounterSharedCasterTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	UGeoEncounterSubsystem* const Subsystem = World->GetSubsystem<UGeoEncounterSubsystem>();
	AHiddenCasterCharacter* const Caster = Subsystem->GetSharedCaster();
	if (TestNotNull(TEXT("The first call spawns the caster"), Caster))
	{
		TestTrue(TEXT("The second returns it"), Subsystem->GetSharedCaster() == Caster);
		int32 Casters = 0;
		for (TActorIterator<AHiddenCasterCharacter> It(World); It; ++It)
		{
			++Casters;
		}

		TestEqual(TEXT("One caster in the world"), Casters, 1);
		TestEqual(TEXT("Enemy team"), Caster->GetGenericTeamId(), FGenericTeamId(static_cast<uint8>(ETeam::Enemy)));
		TestFalse(TEXT("No arena"), Caster->Arena.IsValid());
		TestFalse(TEXT("Nothing can damage it"), Caster->CanBeDamaged());
		TestFalse(TEXT("Nothing can touch it"), Caster->GetActorEnableCollision());
		TestFalse(TEXT("The actor stays shown, so the VFX it owns draw"), Caster->IsHidden());
		TestNull(TEXT("Nothing is drawn: no mesh asset"), Caster->GetMesh()->GetSkeletalMeshAsset());
		TestNull(TEXT("No health bar"), Caster->FindComponentByClass<UWidgetComponent>());
		TestFalse(TEXT("Never aggroed"), Caster->CanBeAggroed());

		UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Caster);
		int32 const SpecsBefore = ASC->GetActivatableAbilities().Num();
		FGameplayAbilitySpecHandle const First = Subsystem->GrantSharedAbility(UGeoTestCastAbility::StaticClass());
		FGameplayAbilitySpecHandle const Second = Subsystem->GrantSharedAbility(UGeoTestCastAbility::StaticClass());
		TestTrue(TEXT("A grant gives the class"), First.IsValid());
		TestTrue(TEXT("Two grants of one class are one spec"), First == Second);
		TestEqual(TEXT("One spec added"), ASC->GetActivatableAbilities().Num(), SpecsBefore + 1);

		AddExpectedErrorPlain(TEXT("->RequiresArena()"), EAutomationExpectedErrorFlags::Contains, 0);
		AddExpectedErrorPlain(TEXT("needs its caster's arena"), EAutomationExpectedErrorFlags::Contains, 1);
		AddExpectedError(TEXT("=== Handled ensure: ===|^\\s*$|^Stack:|\\[Callstack\\]"),
						 EAutomationExpectedErrorFlags::Contains, -1);
		TestFalse(TEXT("A RequiresArena class is refused"),
				  Subsystem->GrantSharedAbility(UGeoTestArenaBoundAbility::StaticClass()).IsValid());
		TestNull(TEXT("Nothing granted"), ASC->FindAbilitySpecFromClass(UGeoTestArenaBoundAbility::StaticClass()));
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityTest, "GeoTrinity.Encounter.ActivateAbility",
								 GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	FGameplayAbilitySpecHandle Handle;
	UGeoAbilitySystemComponent* const ASC = GrantShared(*World, UGeoTestCastAbility::StaticClass(), Handle);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const AfterWait = Encounter->NewAction("AfterWait");
	UGeoTestTrigger* const WaitTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass()), AfterWait});
	UGeoTestAction* const AfterNoWait = Encounter->NewAction("AfterNoWait");
	UGeoTestTrigger* const NoWaitTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass(), false), AfterNoWait});
	Encounter->Start();

	WaitTrigger->FireNow();
	TestTrue(TEXT("The ability runs on the shared caster"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	TestEqual(TEXT("bWaitForEnd holds the next action"), AfterWait->Runs, 0);
	ASC->CancelAbilityHandle(Handle);
	TestEqual(TEXT("It goes on when the ability ends"), AfterWait->Runs, 1);

	NoWaitTrigger->FireNow();
	TestTrue(TEXT("The ability runs again"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	TestEqual(TEXT("Without bWaitForEnd the next action runs at once"), AfterNoWait->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityRefusedTest, "GeoTrinity.Encounter.ActivateAbilityRefused",
								 GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityRefusedTest::RunTest(FString const& /*Parameters*/)
{
	AddExpectedMessage(TEXT("not granted"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1,
					   false);
	AddExpectedMessage(TEXT("already running"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1,
					   false);
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const AfterNoClass = Encounter->NewAction("AfterNoClass");
	UGeoTestTrigger* const NoClassTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, nullptr), AfterNoClass});
	UGeoTestTrigger* const FirstTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass(), false)});
	UGeoTestAction* const AfterRunning = Encounter->NewAction("AfterRunning");
	UGeoTestTrigger* const RunningTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass()), AfterRunning});
	Encounter->Start();

	NoClassTrigger->FireNow();
	TestEqual(TEXT("No ability class: the step goes on"), AfterNoClass->Runs, 1);
	FirstTrigger->FireNow();
	RunningTrigger->FireNow();
	TestEqual(TEXT("The class already running: the step goes on"), AfterRunning->Runs, 1);

	FDataValidationContext Context;
	NewActivateAction(*Encounter, UGeoTestArenaBoundAbility::StaticClass())->IsDataValid(Context);
	TestEqual(TEXT("A RequiresArena class is flagged"), Context.GetNumErrors(), 1u);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityCancelTest, "GeoTrinity.Encounter.ActivateAbilityCancel",
								 GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityCancelTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	FGameplayAbilitySpecHandle Handle;
	UGeoAbilitySystemComponent* const ASC = GrantShared(*World, UGeoTestCastAbility::StaticClass(), Handle);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const CastTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass()), After});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	CastTrigger->FireNow();
	TestTrue(TEXT("The ability runs"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	LeaveTrigger->FireNow();
	TestFalse(TEXT("Leaving the step cancels the ability"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	TestEqual(TEXT("The cancelled wait never reaches the next action"), After->Runs, 0);

	UGeoTestTrigger* const NoWaitTrigger =
		Encounter->AddRule(1, {NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass(), false)});
	UGeoTestTrigger* const NoWaitLeaveTrigger = Encounter->AddRule(1, {}, false, true);
	Encounter->Restart();
	CastTrigger->FireNow();
	LeaveTrigger->FireNow();
	NoWaitTrigger->FireNow();
	NoWaitLeaveTrigger->FireNow();
	TestTrue(TEXT("A step exit leaves an ability fired without waiting running"),
			 ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	Encounter->Abort();
	TestFalse(TEXT("An abort cancels the ability fired without waiting"),
			  ASC->FindAbilitySpecFromHandle(Handle)->IsActive());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityPatternDataTest,
								 "GeoTrinity.Encounter.ActivateAbilityPatternData", GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityPatternDataTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	FGameplayAbilitySpecHandle Handle;
	UGeoAbilitySystemComponent* const ASC = GrantShared(*World, UGeoTestDataPatternAbility::StaticClass(), Handle);
	auto RunningAbility = [ASC, Handle]
	{
		TArray<UGameplayAbility*> const Instances = ASC->FindAbilitySpecFromHandle(Handle)->GetAbilityInstances();
		return Instances.IsEmpty() ? nullptr : Cast<UGeoTestDataPatternAbility>(Instances.Last());
	};

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoActivateAbilityAction* const WithData =
		NewActivateAction(*Encounter, UGeoTestDataPatternAbility::StaticClass(), false);
	FLateralWavePatternData WaveData;
	WaveData.SafeSpotOffsets = {FVector2D(0.f, -300.f), FVector2D(0.f, 500.f)};
	WithData->PatternData = TInstancedStruct<FPatternData>::Make<FLateralWavePatternData>(WaveData);
	UGeoTestTrigger* const WithDataTrigger = Encounter->AddRule(0, {WithData});
	UGeoTestTrigger* const NoDataTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, UGeoTestDataPatternAbility::StaticClass(), false)});
	Encounter->Start();

	WithDataTrigger->FireNow();
	UGeoTestDataPatternAbility const* Ability = RunningAbility();
	FLateralWavePatternData const* const Received =
		Ability ? Ability->LaunchedPatternData.GetPtr<FLateralWavePatternData>() : nullptr;
	if (TestNotNull(TEXT("The action's data reaches the pattern launch"), Received))
	{
		TestTrue(TEXT("With its spots"), Received->SafeSpotOffsets == WaveData.SafeSpotOffsets);
	}

	ASC->CancelAbilityHandle(Handle);
	NoDataTrigger->FireNow();
	Ability = RunningAbility();
	if (TestNotNull(TEXT("The ability runs again"), Ability))
	{
		TestFalse(TEXT("An action without data leaves the launch's unset"), Ability->LaunchedPatternData.IsValid());
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterAbilityRoomTest, "GeoTrinity.Encounter.AbilityRoom",
								 GeoEncounterTestFlags)

bool FGeoEncounterAbilityRoomTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	FGameplayTag const PointTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const RoomTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Tutorial.Zones"));
	FGameplayTag const OtherRoomTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	for (TPair<FGameplayTag, FVector> const& Point :
		 {TPair<FGameplayTag, FVector>(OtherRoomTag, FVector(-700.f, 0.f, 0.f)),
		  TPair<FGameplayTag, FVector>(RoomTag, FVector(400.f, 250.f, 0.f))})
	{
		AGeoTargetPoint* const TargetPoint = World->SpawnActor<AGeoTargetPoint>(Point.Value, FRotator());
		TargetPoint->GameplayTags.AddTag(PointTag);
		TargetPoint->GameplayTags.AddTag(Point.Key);
	}

	FGameplayAbilitySpecHandle Handle;
	UGeoAbilitySystemComponent* const ASC = GrantShared(*World, UGeoTestPointPatternAbility::StaticClass(), Handle);
	CastChecked<UGeoTestPointPatternAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance())
		->SetPointTag(PointTag);
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoActivateAbilityAction* const Action =
		NewActivateAction(*Encounter, UGeoTestPointPatternAbility::StaticClass(), false);
	Action->ArenaTag = RoomTag;
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Action});
	Encounter->Start();

	Trigger->FireNow();
	TArray<UGameplayAbility*> const Instances = ASC->FindAbilitySpecFromHandle(Handle)->GetAbilityInstances();
	UGeoTestPointPatternAbility const* const Ability =
		Instances.IsEmpty() ? nullptr : Cast<UGeoTestPointPatternAbility>(Instances.Last());
	if (TestNotNull(TEXT("The ability runs"), Ability))
	{
		TestEqual(TEXT("It leaves from its point in the action's room, not another room's (X)"),
				  Ability->LaunchedOrigin.X, 400.0);
		TestEqual(TEXT("(Y)"), Ability->LaunchedOrigin.Y, 250.0);
	}

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCharactersFriendlyLookTest, "GeoTrinity.Characters.FriendlyLook",
								 GeoEncounterTestFlags)

bool FGeoCharactersFriendlyLookTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	FGeoEnemySetup AllySetup;
	AllySetup.Team = ETeam::Player;
	FGeoEnemySetup EnemySetup;
	AEnemyCharacter* const Ally = AEnemyCharacter::SpawnEnemy(*World, AEnemyCharacter::StaticClass(),
															  FVector::ZeroVector, nullptr, nullptr, &AllySetup);
	AEnemyCharacter* const Enemy = AEnemyCharacter::SpawnEnemy(*World, AEnemyCharacter::StaticClass(),
															   FVector::ZeroVector, nullptr, nullptr, &EnemySetup);
	if (TestNotNull(TEXT("The ally spawns"), Ally) && TestNotNull(TEXT("The enemy spawns"), Enemy))
	{
		TestTrue(TEXT("A player-team setup gives the friendly look"), Ally->HasFriendlyLook());
		TestFalse(TEXT("An enemy-team setup keeps the enemy look"), Enemy->HasFriendlyLook());

		// A native class's defaults never reach its instances from a changed CDO: the material is set on the actors.
		UMaterialInterface* const FriendlyMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		Ally->FriendlyBodyMaterial = FriendlyMaterial;
		Enemy->FriendlyBodyMaterial = FriendlyMaterial;
		Ally->ApplySetup(AllySetup);
		Enemy->ApplySetup(EnemySetup);
		TestTrue(TEXT("The ally's body wears the friendly material"),
				 Ally->GetMesh()->GetMaterial(0) == FriendlyMaterial);
		TestTrue(TEXT("The enemy's body keeps its own material"), Enemy->GetMesh()->GetMaterial(0) != FriendlyMaterial);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCharactersSetupHealthResetTest, "GeoTrinity.Characters.SetupHealthReset",
								 GeoEncounterTestFlags)

bool FGeoCharactersSetupHealthResetTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());

	FGeoEnemySetup Setup;
	Setup.bResetToFullLife = true;
	Setup.MaxHealth = FScalableFloat(200.f);
	FTransform const Transform;
	AEnemyCharacter* const Enemy =
		World->SpawnActorDeferred<AEnemyCharacter>(AEnemyCharacter::StaticClass(), Transform);
	Enemy->ApplySetup(Setup);
	Enemy->FinishSpawning(Transform);

	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Enemy);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 0.f);
	TestFalse(TEXT("It does not die at zero"), Enemy->IsDead());
	TestEqual(TEXT("It is back at the overridden max health"),
			  ASC->GetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute()), 200.f);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCharactersSetupAggroRadiusTest, "GeoTrinity.Characters.SetupAggroRadius",
								 GeoEncounterTestFlags)

bool FGeoCharactersSetupAggroRadiusTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AEnemyCharacter* const Enemy =
		World->SpawnActorDeferred<AEnemyCharacter>(AEnemyCharacter::StaticClass(), FTransform());
	TestTrue(TEXT("No setup keeps the controller's radius"), Enemy->AggroRadiusOverride < 0.f);

	FGeoEnemySetup Setup;
	TestTrue(TEXT("A setup keeps the controller's radius by default"), Setup.AggroRadius < 0.f);
	Setup.AggroRadius = 100.f;
	Enemy->ApplySetup(Setup);
	TestEqual(TEXT("The setup's radius reaches the enemy"), Enemy->AggroRadiusOverride, 100.f);

	Enemy->Destroy();
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

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSpawnZoneEnteredTest, "GeoTrinity.Encounter.SpawnZoneEntered",
								 GeoEncounterTestFlags)

bool FGeoEncounterSpawnZoneEnteredTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoEffectZone::StaticClass();

	FGameplayTag const PurposeTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("TargetPoint.ZoneSpawn"));
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector::ZeroVector, FRotator());
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	FTransform const Transform(FVector(2000.f, 0.f, 0.f));
	APlayableCharacter* const Player =
		World->SpawnActorDeferred<APlayableCharacter>(APlayableCharacter::StaticClass(), Transform);
	Player->SetActorHiddenInGame(true);
	Player->FinishSpawning(Transform);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoSpawnZoneAction* const Spawner = NewObject<UGeoSpawnZoneAction>(Encounter);
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->Params.Size = 300.f;
	Spawner->EnteredEvent = FGameplayTag::RequestGameplayTag("Encounter.Event.ZoneShield");
	UGeoTestTrigger* const SpawnTrigger = Encounter->AddRule(0, {Spawner});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	int32 Entries = 0;
	World->GetSubsystem<UGeoEncounterSubsystem>()->OnRaised.AddLambda(
		[&Entries, Player](FGeoEncounterPayload const& Payload)
		{
			Entries += Payload.Actor == Player ? 1 : 0;
		});

	SpawnTrigger->FireNow();
	TestEqual(TEXT("Outside at spawn: nothing raised"), Entries, 0);
	// No default attributes in a test world: the zone has no health and blinks, which turns its collision off.
	(*TActorIterator<AGeoEffectZone>(World))->SetActorEnableCollision(true);
	Player->SetActorLocation(FVector::ZeroVector);
	TestEqual(TEXT("Entering raises"), Entries, 1);
	Player->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	Player->SetActorLocation(FVector::ZeroVector);
	TestEqual(TEXT("Leaving and entering again raises again"), Entries, 2);

	LeaveTrigger->FireNow();
	Player->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	Player->SetActorLocation(FVector::ZeroVector);
	TestEqual(TEXT("After the step is left: nothing"), Entries, 2);

	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSetTeleporterOpenTest, "GeoTrinity.Encounter.SetTeleporterOpen",
								 GeoEncounterTestFlags)

bool FGeoEncounterSetTeleporterOpenTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTeleporter* const Teleporter = World->SpawnActor<AGeoTeleporter>();
	UStaticMeshComponent* const Mesh = Teleporter->FindComponentByClass<UStaticMeshComponent>();
	TestTrue(TEXT("A pad is open by default"),
			 Teleporter->IsOpen() && Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	UGeoSetTeleporterOpenAction* const Action = NewObject<UGeoSetTeleporterOpenAction>(Encounter);
	Action->Teleporter = Teleporter;
	Action->bOpen = false;
	Action->Execute_Implementation(FGeoEncounterPayload());
	TestFalse(TEXT("Closed"), Teleporter->IsOpen());
	TestEqual(TEXT("A closed pad has no overlap"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);

	Action->bOpen = true;
	Action->Execute_Implementation(FGeoEncounterPayload());
	TestTrue(TEXT("Open again"), Teleporter->IsOpen());
	TestEqual(TEXT("Overlap is back"), Mesh->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSetMovementLockedTest, "GeoTrinity.Encounter.SetMovementLocked",
								 GeoEncounterTestFlags)

bool FGeoEncounterSetMovementLockedTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	UGeoSetMovementLockedAction* const Action = NewObject<UGeoSetMovementLockedAction>(Encounter);
	Action->Execute_Implementation(FGeoEncounterPayload());
	TestTrue(TEXT("Locked"), PlayerController->IsMoveInputIgnored());

	Action->bLocked = false;
	Action->Execute_Implementation(FGeoEncounterPayload());
	TestFalse(TEXT("Unlocked"), PlayerController->IsMoveInputIgnored());

	Action->bLocked = true;
	Action->Execute_Implementation(FGeoEncounterPayload());
	Action->Execute_Implementation(FGeoEncounterPayload());
	Action->bLocked = false;
	Action->Execute_Implementation(FGeoEncounterPayload());
	TestFalse(TEXT("A lock while locked adds nothing: one unlock frees"), PlayerController->IsMoveInputIgnored());

	PlayerController->SetIgnoreMoveInput(true);
	Action->Execute_Implementation(FGeoEncounterPayload());
	TestTrue(TEXT("An unlock while free leaves another system's lock alone"), PlayerController->IsMoveInputIgnored());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterMovementLockRestartTest, "GeoTrinity.Encounter.MovementLockRestart",
								 GeoEncounterTestFlags)

bool FGeoEncounterMovementLockRestartTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoSetMovementLockedAction* const Unlock = NewObject<UGeoSetMovementLockedAction>(Encounter);
	Unlock->bLocked = false;
	Encounter->EditSteps()[0].OnEnter = {NewObject<UGeoSetMovementLockedAction>(Encounter)};
	Encounter->EditSteps()[0].OnExit = {Unlock};
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();
	TestTrue(TEXT("The step locks on entry"), PlayerController->IsMoveInputIgnored());

	Encounter->Restart();
	TestTrue(TEXT("Restarted: still locked"), PlayerController->IsMoveInputIgnored());
	LeaveTrigger->FireNow();
	TestFalse(TEXT("The step's one unlock frees it after a restart"), PlayerController->IsMoveInputIgnored());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterJumpSkipOrderTest, "GeoTrinity.Encounter.JumpSkipOrder",
								 GeoEncounterTestFlags)

bool FGeoEncounterJumpSkipOrderTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	TArray<FName> Order;
	AGeoTestEncounter* const EncounterA = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const EncounterB = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const EncounterC = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const OffChain = World->SpawnActor<AGeoTestEncounter>();
	EncounterA->SetStartOnBeginPlay(true);

	EncounterA->AddSteps(2);
	EncounterA->EditSteps()[0].OnEnter = {EncounterA->NewAction("EnterA0", &Order)};
	EncounterA->EditSteps()[0].OnExit = {EncounterA->NewAction("ExitA0", &Order)};
	EncounterA->EditSteps()[1].OnEnter = {EncounterA->NewAction("EnterA1", &Order)};
	UGeoTestAction* const RuleActionA = EncounterA->NewAction("RuleA", &Order);
	EncounterA->AddRule(0, {RuleActionA});
	EncounterA->AddRule(1, {EncounterA->NewStartAction(EncounterB)}, false, true);

	EncounterB->AddSteps(1);
	EncounterB->EditSteps()[0].OnEnter = {EncounterB->NewAction("EnterB0", &Order)};
	EncounterB->EditSteps()[0].OnExit = {EncounterB->NewAction("ExitB0", &Order)};
	EncounterB->AddRule(0, {EncounterB->NewStartAction(EncounterC)}, false, true);

	EncounterC->AddSteps(1);
	UGeoTestAction* const EnterC = EncounterC->NewAction("EnterC", &Order);
	EncounterC->EditSteps()[0].OnEnter = {EnterC};
	EncounterC->AddRule(0, {});

	OffChain->AddSteps(1);
	OffChain->EditSteps()[0].OnEnter = {OffChain->NewAction("EnterOff", &Order)};
	OffChain->AddRule(0, {});

	EncounterC->StartAfterSkippingChain();
	TestTrue(TEXT("Skipped encounters in chain order, each step's OnEnter then OnExit, then the target started"),
			 Order == TArray<FName>{"EnterA0", "ExitA0", "EnterA1", "EnterB0", "ExitB0", "EnterC"});
	TestEqual(TEXT("Rule actions are never skipped"), RuleActionA->Skips + RuleActionA->Runs, 0);
	TestEqual(TEXT("The target runs"), EnterC->Runs, 1);
	TestEqual(TEXT("The target is not skipped"), EnterC->Skips, 0);

	Order.Reset();
	OffChain->StartAfterSkippingChain();
	TestTrue(TEXT("Off the chain: nothing skipped, started anyway"), Order == TArray<FName>{"EnterOff"});

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterJumpPortalTest, "GeoTrinity.Encounter.JumpPortal", GeoEncounterTestFlags)

bool FGeoEncounterJumpPortalTest::RunTest(FString const& /*Parameters*/)
{
	/** A chain Landing -> Dash -> Shoot spawned deferred under Portal, so their BeginPlay sees it; returns the Order of
	 * the OnEnter actions run or skipped, and whether the pawn ended on Shoot's jump start. */
	auto const Jump = [](FString const& Portal, bool& bPawnOnShootStart)
	{
		UWorld* World = CreateTestWorld();
		World->URL.Portal = Portal;
		APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
		APawn* const Pawn = World->SpawnActor<ADefaultPawn>();
		PlayerController->Possess(Pawn);
		APlayerStart* const ShootStart =
			World->SpawnActor<APlayerStart>(FVector(5000.f, 0.f, 100.f), FRotator::ZeroRotator);
		ShootStart->PlayerStartTag = TEXT("Shoot");

		TArray<FName> Order;
		TArray<AGeoTestEncounter*> Rooms;
		for (FName const Room : {FName("Landing"), FName("Dash"), FName("Shoot")})
		{
			AGeoTestEncounter* const Encounter =
				World->SpawnActorDeferred<AGeoTestEncounter>(AGeoTestEncounter::StaticClass(), FTransform::Identity);
			Encounter->AddSteps(1);
			Encounter->EditSteps()[0].OnEnter = {Encounter->NewAction(Room, &Order)};
			Rooms.Add(Encounter);
		}

		Rooms[0]->SetStartOnBeginPlay(true);
		Rooms[0]->AddRule(0, {Rooms[0]->NewStartAction(Rooms[1])});
		Rooms[1]->AddRule(0, {Rooms[1]->NewStartAction(Rooms[2])});
		Rooms[2]->AddRule(0, {});
		Rooms[2]->SetJumpStart(ShootStart);
		for (AGeoTestEncounter* const Encounter : Rooms)
		{
			Encounter->FinishSpawning(FTransform::Identity);
		}

		bPawnOnShootStart = Pawn->GetActorLocation().Equals(ShootStart->GetActorLocation(), 1.f);
		DestroyTestWorld(World);
		return Order;
	};

	bool bPawnOnShootStart = false;
	TestTrue(TEXT("Number 2: Landing and Dash skipped, Shoot started"),
			 Jump(TEXT("2"), bPawnOnShootStart) == TArray<FName>{"Landing", "Dash", "Shoot"});
	TestTrue(TEXT("Number 2: the pawn is moved onto Shoot's start"), bPawnOnShootStart);
	TestTrue(TEXT("Name Shoot: same jump"),
			 Jump(TEXT("Shoot"), bPawnOnShootStart) == TArray<FName>{"Landing", "Dash", "Shoot"});
	TestTrue(TEXT("Number 0: the normal start"), Jump(TEXT("0"), bPawnOnShootStart) == TArray<FName>{"Landing"});
	TestTrue(TEXT("Number past the chain: the normal start"),
			 Jump(TEXT("7"), bPawnOnShootStart) == TArray<FName>{"Landing"});
	TestTrue(TEXT("Unknown name: the normal start"),
			 Jump(TEXT("Nowhere"), bPawnOnShootStart) == TArray<FName>{"Landing"});
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterJumpSkipLockTest, "GeoTrinity.Encounter.JumpSkipLock",
								 GeoEncounterTestFlags)

bool FGeoEncounterJumpSkipLockTest::RunTest(FString const& /*Parameters*/)
{
	auto const MovementIgnoredAfterSkip = [](bool const bWithUnlock)
	{
		UWorld* World = CreateTestWorld();
		APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
		AGeoTestEncounter* const Landing = World->SpawnActor<AGeoTestEncounter>();
		AGeoTestEncounter* const Next = World->SpawnActor<AGeoTestEncounter>();
		Landing->SetStartOnBeginPlay(true);
		Landing->AddSteps(1);
		Landing->EditSteps()[0].OnEnter = {NewObject<UGeoSetMovementLockedAction>(Landing)};
		if (bWithUnlock)
		{
			UGeoSetMovementLockedAction* const Unlock = NewObject<UGeoSetMovementLockedAction>(Landing);
			Unlock->bLocked = false;
			Landing->EditSteps()[0].OnExit = {Unlock};
		}

		Landing->AddRule(0, {Landing->NewStartAction(Next)}, false, true);
		Next->AddSteps(1);
		Next->AddRule(0, {});

		Next->StartAfterSkippingChain();
		bool const bIgnored = PlayerController->IsMoveInputIgnored();
		DestroyTestWorld(World);
		return bIgnored;
	};

	TestTrue(TEXT("A skipped lock locks"), MovementIgnoredAfterSkip(false));
	TestFalse(TEXT("Lock then unlock, skipped in order, leaves movement free"), MovementIgnoredAfterSkip(true));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterArrivalAbortTest, "GeoTrinity.Encounter.ArrivalAbort",
								 GeoEncounterTestFlags)

bool FGeoEncounterArrivalAbortTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	TArray<FName> Order;
	AGeoTestEncounter* const Left = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const Arrived = World->SpawnActor<AGeoTestEncounter>();
	Left->SetStartOnBeginPlay(true);

	Left->AddSteps(1);
	UGeoTestLatentAction* const Waiting = Left->NewAction<UGeoTestLatentAction>("Waiting", &Order);
	UGeoTestAction* const Finished = Left->NewAction("Finished", &Order);
	UGeoTestAction* const LeftExit = Left->NewAction("LeftExit", &Order);
	Left->EditSteps()[0].OnExit = {LeftExit};
	UGeoTestTrigger* const FinishedTrigger = Left->AddRule(0, {Finished});
	UGeoTestTrigger* const WaitingTrigger = Left->AddRule(0, {Waiting});
	Left->AddRule(0, {Left->NewStartAction(Arrived)});
	UGeoTestCompletionListener* const Listener = NewObject<UGeoTestCompletionListener>(Left);
	Left->OnCompleted.AddDynamic(Listener, &UGeoTestCompletionListener::OnCompleted);

	Arrived->AddSteps(1);
	UGeoTestAction* const EnterArrived = Arrived->NewAction("EnterArrived", &Order);
	Arrived->EditSteps()[0].OnEnter = {EnterArrived};
	Arrived->AddRule(0, {});

	Left->Start();
	FinishedTrigger->FireNow();
	WaitingTrigger->FireNow();
	Arrived->StartAfterSkippingChain();
	TestTrue(TEXT("The running action of the left room is cancelled"), Waiting->Cancels > 0);
	TestTrue(TEXT("A finished action of the left room is cancelled too"), Finished->Cancels > 0);
	TestEqual(TEXT("No OnExit for the left room"), LeftExit->Runs, 0);
	TestEqual(TEXT("The left room's trigger is disarmed"), FinishedTrigger->DisarmCount, 1);
	TestEqual(TEXT("An abort is not a completion"), Listener->Completions, 0);
	TestEqual(TEXT("The arrived room starts once"), EnterArrived->Runs, 1);

	FinishedTrigger->FireNow();
	TestEqual(TEXT("The left room reacts to nothing any more"), Finished->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterArrivalFromOwnRuleTest, "GeoTrinity.Encounter.ArrivalFromOwnRule",
								 GeoEncounterTestFlags)

bool FGeoEncounterArrivalFromOwnRuleTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	TArray<FName> Order;
	AGeoTestEncounter* const Left = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const Arrived = World->SpawnActor<AGeoTestEncounter>();
	Left->SetStartOnBeginPlay(true);

	Left->AddSteps(2);
	UGeoTestAction* const AfterStart = Left->NewAction("AfterStart", &Order);
	UGeoTestAction* const LeftExit = Left->NewAction("LeftExit", &Order);
	UGeoTestAction* const LeftSecondStep = Left->NewAction("LeftSecondStep", &Order);
	Left->EditSteps()[0].OnExit = {LeftExit};
	Left->EditSteps()[1].OnEnter = {LeftSecondStep};
	UGeoTestTrigger* const Trigger = Left->AddRule(0, {Left->NewStartAction(Arrived), AfterStart}, false, true);
	Left->AddRule(1, {});
	UGeoTestCompletionListener* const Listener = NewObject<UGeoTestCompletionListener>(Left);
	Left->OnCompleted.AddDynamic(Listener, &UGeoTestCompletionListener::OnCompleted);

	Arrived->AddSteps(1);
	UGeoTestAction* const EnterArrived = Arrived->NewAction("EnterArrived", &Order);
	Arrived->EditSteps()[0].OnEnter = {EnterArrived};
	Arrived->AddRule(0, {});

	Left->Start();
	Trigger->FireNow();
	TestEqual(TEXT("The arrived room starts once"), EnterArrived->Runs, 1);
	TestEqual(TEXT("The rest of the aborted rule never runs"), AfterStart->Runs, 0);
	TestEqual(TEXT("The completion the rule asked is dropped: no OnExit"), LeftExit->Runs, 0);
	TestEqual(TEXT("The completion the rule asked is dropped: no next step"), LeftSecondStep->Runs, 0);
	TestEqual(TEXT("An abort is not a completion"), Listener->Completions, 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterArrivalCancelUndoesTest, "GeoTrinity.Encounter.ArrivalCancelUndoes",
								 GeoEncounterTestFlags)

bool FGeoEncounterArrivalCancelUndoesTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestHUD* const HUD = World->SpawnActor<AGeoTestHUD>();
	PlayerController->MyHUD = HUD;
	AGeoCat* const Cat = World->SpawnActor<AGeoCat>();
	Cat->SetActorHiddenInGame(true);
	AActor* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(500.f, 0.f, 0.f), FRotator());

	AGeoTestEncounter* const Left = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const Arrived = World->SpawnActor<AGeoTestEncounter>();
	Left->SetStartOnBeginPlay(true);

	Left->AddSteps(1);
	UGeoQuestCardAction* const Card = NewObject<UGeoQuestCardAction>(Left);
	UGeoSetActorHiddenAction* const ShowCat = NewObject<UGeoSetActorHiddenAction>(Left);
	ShowCat->Actor = Cat;
	ShowCat->bHidden = false;
	UGeoCatWalkAction* const Walk = NewObject<UGeoCatWalkAction>(Left);
	Walk->Cat = Cat;
	Walk->Path = {Point};
	Walk->bWaitForEnd = false;
	UGeoCatLineAction* const Line = NewObject<UGeoCatLineAction>(Left);
	Line->Line = INVTEXT("Line");
	UGeoTestTrigger* const Trigger = Left->AddRule(0, {Card, ShowCat, Walk, Line}, true);
	Left->AddRule(0, {Left->NewStartAction(Arrived)});

	Arrived->AddSteps(1);
	Arrived->AddRule(0, {});
	UGeoCatLineAction* const NeverRun = NewObject<UGeoCatLineAction>(Arrived);
	UGeoQuestCardAction* const NeverShown = NewObject<UGeoQuestCardAction>(Arrived);
	UGeoSetActorHiddenAction* const NeverShowed = NewObject<UGeoSetActorHiddenAction>(Arrived);
	NeverShowed->Actor = Cat;
	NeverShowed->bHidden = false;

	Left->Start();
	Trigger->FireNow();
	TestTrue(TEXT("The card, the cat and the line show"),
			 HUD->bQuestCardShown && !Cat->IsHidden() && HUD->Lines.Num() == 1);

	Left->Restart();
	TestEqual(TEXT("A normal step exit leaves the line standing"), HUD->HiddenLines, 0);

	Trigger->FireNow();
	NeverRun->OnCancel();
	NeverShown->OnCancel();
	NeverShowed->OnCancel();
	TestTrue(TEXT("Cancelling actions that never ran undoes nothing"),
			 HUD->HiddenLines == 0 && HUD->bQuestCardShown && !Cat->IsHidden());

	Arrived->StartAfterSkippingChain();
	TestEqual(TEXT("The abort ends the line"), HUD->HiddenLines, 1);
	TestFalse(TEXT("The abort hides the card"), HUD->bQuestCardShown);
	TestTrue(TEXT("The abort hides the cat again"), Cat->IsHidden());
	TestFalse(TEXT("The abort stops the cat's order"), Cat->IsOrderRunning());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterArrivalSkipReplayTest, "GeoTrinity.Encounter.ArrivalSkipReplay",
								 GeoEncounterTestFlags)

bool FGeoEncounterArrivalSkipReplayTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestEncounter* const Left = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const Arrived = World->SpawnActor<AGeoTestEncounter>();
	Left->SetStartOnBeginPlay(true);

	Left->AddSteps(1);
	UGeoSetMovementLockedAction* const Unlock = NewObject<UGeoSetMovementLockedAction>(Left);
	Unlock->bLocked = false;
	Left->EditSteps()[0].OnEnter = {NewObject<UGeoSetMovementLockedAction>(Left)};
	Left->EditSteps()[0].OnExit = {Unlock};
	Left->AddRule(0, {Left->NewStartAction(Arrived)});
	Arrived->AddSteps(1);
	Arrived->AddRule(0, {});

	Left->Start();
	TestTrue(TEXT("The left room locked movement"), PlayerController->IsMoveInputIgnored());

	Arrived->StartAfterSkippingChain();
	TestFalse(TEXT("Aborted without OnExit, the replayed unlock still frees movement"),
			  PlayerController->IsMoveInputIgnored());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterSessionEndTest, "GeoTrinity.Encounter.SessionEnd", GeoEncounterTestFlags)

bool FGeoEncounterSessionEndTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestLatentAction* const Waiting = Encounter->NewAction<UGeoTestLatentAction>("Waiting");
	UGeoTestAction* const Next = Encounter->NewAction("Next");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Waiting, Next});
	Encounter->Start();
	Trigger->FireNow();

	FWorldDelegates::OnWorldBeginTearDown.Broadcast(World);
	TestTrue(TEXT("The session end cancels the awaited action"), Waiting->Cancels > 0);
	TestEqual(TEXT("The encounter disarms its trigger"), Trigger->DisarmCount, 1);

	Waiting->Finish();
	TestEqual(TEXT("A Finish after the session end runs no next action"), Next->Runs, 0);

	int32 const CancelsAfterFirstEnd = Waiting->Cancels;
	FWorldDelegates::OnWorldBeginTearDown.Broadcast(World);
	TestEqual(TEXT("A second session end aborts nothing twice"), Waiting->Cancels, CancelsAfterFirstEnd);

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableSelfOwnedSpawnTest, "GeoTrinity.Deployable.SelfOwnedSpawn",
								 GeoEncounterTestFlags)

bool FGeoDeployableSelfOwnedSpawnTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	FDeployableData Data;
	Data.Params.LifeDrainMaxDuration = 10.f;
	AGeoDeployableBase* const Deployable =
		GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform::Identity, World);
	if (TestNotNull(TEXT("A deployable with no owner spawns"), Deployable))
	{
		TestTrue(TEXT("It is its own owner"), Data.Owner == Deployable);
		TestTrue(TEXT("And its own instigator"), Data.Instigator == Deployable);
		TestTrue(TEXT("It is active, with its own ability system"),
				 Deployable->IsActive() && Deployable->GetAbilitySystemComponent() != nullptr);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterKillOnStepExitTest, "GeoTrinity.Encounter.KillOnStepExit",
								 GeoEncounterTestFlags)

bool FGeoEncounterKillOnStepExitTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	FGameplayTag const PurposeTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const Point = World->SpawnActor<AGeoTargetPoint>(FVector::ZeroVector, FRotator());
	Point->GameplayTags.AddTag(PurposeTag);
	Point->GameplayTags.AddTag(ArenaTag);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(3);
	UGeoTestAction* const EnterB = Encounter->NewAction("EnterB");
	UGeoTestAction* const EnterC = Encounter->NewAction("EnterC");
	Encounter->EditSteps()[1].OnEnter = {EnterB};
	Encounter->EditSteps()[2].OnEnter = {EnterC};
	UGeoSpawnEnemiesAction* const Spawner = NewObject<UGeoSpawnEnemiesAction>(Encounter);
	Spawner->EnemyClass = AEnemyCharacter::StaticClass();
	Spawner->Count = 2;
	Spawner->PurposeTag = PurposeTag;
	Spawner->ArenaTag = ArenaTag;
	Spawner->bKillOnStepExit = true;
	Encounter->EditSteps()[0].OnEnter = {Spawner};

	UGeoActorDiedTrigger* const QuipTrigger = NewObject<UGeoActorDiedTrigger>(Encounter);
	QuipTrigger->Watched.Class = AEnemyCharacter::StaticClass();
	UGeoTestAction* const Quip = Encounter->NewAction("Quip");
	Encounter->AddRuleWithTrigger(0, QuipTrigger, {Quip}, true);
	UGeoActorDiedTrigger* const AllDeadTrigger = NewObject<UGeoActorDiedTrigger>(Encounter);
	AllDeadTrigger->Watched.Class = AEnemyCharacter::StaticClass();
	FGeoEncounterRule& AllDeadRule = Encounter->EditSteps()[0].Rules.AddDefaulted_GetRef();
	AllDeadRule.Trigger = AllDeadTrigger;
	AllDeadRule.Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 2.f);
	AllDeadRule.bCompletesStep = true;
	AllDeadRule.StepAfter = TEXT("C");
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	TActorIterator<AEnemyCharacter> const First(World);
	if (TestTrue(TEXT("The enemies spawn"), static_cast<bool>(First)))
	{
		First->Death();
		TestEqual(TEXT("A death in the step fires its rule"), Quip->Runs, 1);
		LeaveTrigger->FireNow();
		TestEqual(TEXT("The deaths the cancel causes fire no rule of the step left"), Quip->Runs, 1);
		TestEqual(TEXT("The step completes where it was told"), EnterB->Runs, 1);
		TestEqual(TEXT("Not where a death rule of the left step would send it"), EnterC->Runs, 0);
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

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterEnterDelayTest, "GeoTrinity.Encounter.EnterDelay", GeoEncounterTestFlags)

bool FGeoEncounterEnterDelayTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoDelayAction* const Delay = NewObject<UGeoDelayAction>(Encounter);
	Delay->Duration = 1.f;
	UGeoTestAction* const Reaction = Encounter->NewAction("Reaction");
	Encounter->EditSteps()[0].OnEnter = {Delay, Reaction};
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 0.5f);
	TestEqual(TEXT("OnEnter waits for its delay action"), Reaction->Runs, 0);
	TickTestTimers(*World, 0.51f);
	TestEqual(TEXT("Runs after the delay"), Reaction->Runs, 1);

	Encounter->Restart();
	TickTestTimers(*World, 0.f);
	LeaveTrigger->FireNow();
	TickTestTimers(*World, 2.f);
	TestEqual(TEXT("Leaving the step cancels the delay"), Reaction->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterCatLineTest, "GeoTrinity.Encounter.CatLine", GeoEncounterTestFlags)

bool FGeoEncounterCatLineTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestHUD* const HUD = World->SpawnActor<AGeoTestHUD>();
	PlayerController->MyHUD = HUD;

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoCatLineAction* const First = NewObject<UGeoCatLineAction>(Encounter);
	First->Line = INVTEXT("First");
	UGeoCatLineAction* const Second = NewObject<UGeoCatLineAction>(Encounter);
	Second->Line = INVTEXT("Second");
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {First, Second, After}, true);
	Encounter->Start();

	Trigger->FireNow();
	TestTrue(TEXT("The first line shows"), HUD->Lines == TArray<FString>{"First"});
	FSimpleDelegate const CancelledLineEnd = HUD->LineEnded;
	Encounter->Restart();
	Trigger->FireNow();
	CancelledLineEnd.ExecuteIfBound();
	TestEqual(TEXT("The end of a line shown by a cancelled run finishes nothing"), HUD->Lines.Num(), 2);

	HUD->LineEnded.ExecuteIfBound();
	TestTrue(TEXT("The next line waits for the end of the one before"),
			 HUD->Lines == TArray<FString>{"First", "First", "Second"});
	TestEqual(TEXT("Not before the last line ends"), After->Runs, 0);
	HUD->LineEnded.ExecuteIfBound();
	TestEqual(TEXT("The run goes on once the last line ends"), After->Runs, 1);

	UGeoCatLineAction* const Counting = NewObject<UGeoCatLineAction>(Encounter);
	Counting->Line = INVTEXT("That's {Counter:Falls} deaths, {Counter:Falls}. {Counter:Unset} {Key:X}");
	Encounter->AddToEncounterCounter("Falls", 3);
	TArray<FString> BroadcastLines;
	World->GetSubsystem<UGeoEncounterSubsystem>()->OnCatLine.AddLambda(
		[&BroadcastLines](FText const& Line)
		{
			BroadcastLines.Add(Line.ToString());
		});
	Counting->Begin(FGeoEncounterPayload());
	TestEqual(TEXT("A counter token reads its encounter's counter, an unset one is 0, other tokens stay"),
			  HUD->Lines.Last(), FString("That's 3 deaths, 3. 0 {Key:X}"));
	TestTrue(TEXT("OnCatLine got the replaced text"), BroadcastLines == TArray<FString>{HUD->Lines.Last()});

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterCatLineFromFileTest, "GeoTrinity.Encounter.CatLineFromFile",
								 GeoEncounterTestFlags)

bool FGeoEncounterCatLineFromFileTest::RunTest(FString const& /*Parameters*/)
{
	FString const Path = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("CatLines"), TEXT(".txt"));
	FFileHelper::SaveStringToFile(TEXT("[Falls]\nThat's {Counter:Falls} deaths.\n[Other]\nnever\n"), *Path,
								  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	GeoCatLines::SetFilePath(Path);

	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestHUD* const HUD = World->SpawnActor<AGeoTestHUD>();
	PlayerController->MyHUD = HUD;
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	Encounter->AddToEncounterCounter("Falls", 3);

	UGeoCatLineFromFileAction* const FromFile = NewObject<UGeoCatLineFromFileAction>(Encounter);
	FromFile->Categories.Names = {"Falls"};
	FromFile->Begin(FGeoEncounterPayload());
	TestTrue(TEXT("A line of the picked category shows, its counter token replaced"),
			 HUD->Lines == TArray<FString>{"That's 3 deaths."});

	FromFile->bRandom = false;
	HUD->LineEnded.ExecuteIfBound();
	FromFile->Begin(FGeoEncounterPayload());
	TestEqual(TEXT("File order shows a line too"), HUD->Lines.Last(), FString("That's 3 deaths."));

	UGeoCatLineFromFileAction* const NoMatch = NewObject<UGeoCatLineFromFileAction>(Encounter);
	NoMatch->Categories.Names = {"Nope"};
	int32 const ShownBefore = HUD->Lines.Num();
	NoMatch->Begin(FGeoEncounterPayload());
	TestEqual(TEXT("No line in the picked categories shows nothing"), HUD->Lines.Num(), ShownBefore);

	DestroyTestWorld(World);
	GeoCatLines::SetFilePath(FString());
	IFileManager::Get().Delete(*Path);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterAbilityPageTest, "GeoTrinity.Encounter.AbilityPage",
								 GeoEncounterTestFlags)

bool FGeoEncounterAbilityPageTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestHUD* const HUD = World->SpawnActor<AGeoTestHUD>();
	PlayerController->MyHUD = HUD;

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoShowAbilityPageAction* const Page = NewObject<UGeoShowAbilityPageAction>(Encounter);
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Page, After}, true);
	Encounter->Start();

	Trigger->FireNow();
	TestTrue(TEXT("The page opens"), HUD->PageClosed.IsBound());
	TestEqual(TEXT("What follows waits while the page is open"), After->Runs, 0);

	FSimpleDelegate const CancelledClose = HUD->PageClosed;
	Encounter->Restart();
	Trigger->FireNow();
	CancelledClose.ExecuteIfBound();
	TestEqual(TEXT("The close of a page opened by a cancelled run finishes nothing"), After->Runs, 0);

	HUD->PageClosed.ExecuteIfBound();
	TestEqual(TEXT("The run goes on once the player closes the page"), After->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterCatActionTest, "GeoTrinity.Encounter.CatAction", GeoEncounterTestFlags)

bool FGeoEncounterCatActionTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoCat* const Cat = World->SpawnActor<AGeoCat>();
	AActor* const Point = World->SpawnActor<AGeoTargetPoint>(FVector(500.f, 0.f, 0.f), FRotator());

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoCatWalkAction* const Walk = NewObject<UGeoCatWalkAction>(Encounter);
	Walk->Cat = Cat;
	Walk->Path = {Point};
	UGeoTestAction* const AfterWalk = Encounter->NewAction("AfterWalk");
	UGeoCatWalkAction* const WalkNoWait = NewObject<UGeoCatWalkAction>(Encounter);
	WalkNoWait->Cat = Cat;
	WalkNoWait->Path = {Point};
	WalkNoWait->bWaitForEnd = false;
	UGeoTestAction* const AfterNoWait = Encounter->NewAction("AfterNoWait");
	UGeoCatRoamAction* const Roam = NewObject<UGeoCatRoamAction>(Encounter);
	Roam->Cat = Cat;
	Roam->Settings.Area = Point;
	UGeoCatMontageAction* const NoCat = NewObject<UGeoCatMontageAction>(Encounter);
	UGeoTestAction* const AfterRoam = Encounter->NewAction("AfterRoam");
	UGeoCatMontageAction* const Montage = NewObject<UGeoCatMontageAction>(Encounter);
	Montage->Cat = Cat;
	UGeoSetActorHiddenAction* const Hide = NewObject<UGeoSetActorHiddenAction>(Encounter);
	Hide->Actor = Cat;
	UGeoTestTrigger* const WaitTrigger = Encounter->AddRule(0, {Walk, AfterWalk});
	UGeoTestTrigger* const NoWaitTrigger = Encounter->AddRule(0, {WalkNoWait, AfterNoWait});
	UGeoTestTrigger* const RoamTrigger = Encounter->AddRule(0, {Roam, NoCat, AfterRoam});
	UGeoTestTrigger* const JumpTrigger = Encounter->AddRule(0, {Montage, Hide});
	Encounter->Start();

	WaitTrigger->FireNow();
	TestEqual(TEXT("bWaitForEnd: waits for the cat"), AfterWalk->Runs, 0);
	Cat->AdvanceMove(100.f);
	TestEqual(TEXT("Finishes when the walk ends"), AfterWalk->Runs, 1);

	Cat->SetActorLocation(FVector::ZeroVector);
	NoWaitTrigger->FireNow();
	TestEqual(TEXT("bWaitForEnd false: finishes at once"), AfterNoWait->Runs, 1);
	TestTrue(TEXT("While the cat still walks"), Cat->IsOrderRunning());

	AddExpectedMessage(TEXT("no Cat set"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	RoamTrigger->FireNow();
	TestEqual(TEXT("Roam finishes at once, a null cat too"), AfterRoam->Runs, 1);
	TestTrue(TEXT("The cat roams"), Cat->IsOrderRunning());

	JumpTrigger->FireNow();
	TestTrue(TEXT("A montage then a hide: hidden once the montage order ends"), Cat->IsHidden());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoGrowingZoneTest, "GeoTrinity.Deployable.GrowingZone", GeoEncounterTestFlags)

bool FGeoGrowingZoneTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	FDeployableData Data;
	Data.Params.Size = 100.f;
	AGeoTestReadableZone* const FixedZone = Cast<AGeoTestReadableZone>(GeoASLib::FullySpawnDeployable(
		AGeoTestReadableZone::StaticClass(), Data, FTransform(FVector(2000.f, 0.f, 0.f)), World));
	Data.Owner = nullptr;
	Data.Instigator = nullptr;
	Data.Params.GrowToSize = FScalableFloat(600.f);
	Data.Params.GrowSeconds = FScalableFloat(10.f);
	AGeoTestReadableZone* const Zone = Cast<AGeoTestReadableZone>(
		GeoASLib::FullySpawnDeployable(AGeoTestReadableZone::StaticClass(), Data, FTransform::Identity, World));
	if (TestNotNull(TEXT("The zones spawn"), Zone) && FixedZone)
	{
		TestEqual(TEXT("It starts at its spawn size"), Zone->GetTestRadius(), 100.f);
		World->TimeSeconds += 5.f;
		Zone->Tick(0.f);
		FixedZone->Tick(0.f);
		TestEqual(TEXT("Half of GrowSeconds: half way to GrowToSize"), Zone->GetTestRadius(), 350.f, 0.01f);
		TestEqual(TEXT("The capsule follows"),
				  Zone->FindComponentByClass<UCapsuleComponent>()->GetUnscaledCapsuleRadius(), 350.f, 0.01f);
		TestEqual(TEXT("The replicated spawn size never changes"), Zone->GetSpawnSize(), 100.f);
		TestEqual(TEXT("GrowToSize 0: it keeps its size"), FixedZone->GetTestRadius(), 100.f);
		World->TimeSeconds += 60.f;
		Zone->Tick(0.f);
		TestEqual(TEXT("It stops at GrowToSize"), Zone->GetTestRadius(), 600.f, 0.01f);

		World->TimeSeconds = 100.f;
		Data.Owner = nullptr;
		Data.Instigator = nullptr;
		AGeoTestReadableZone* const LateZone = Cast<AGeoTestReadableZone>(
			GeoASLib::FullySpawnDeployable(AGeoTestReadableZone::StaticClass(), Data, FTransform::Identity, World));
		if (TestNotNull(TEXT("The late zone spawns"), LateZone))
		{
			TestEqual(TEXT("The growth starts at the server time of its BeginPlay"),
					  LateZone->GetGrowthStartServerTime(), 100.f);
			LateZone->ReceiveGrowthStartServerTime(92.f);
			World->TimeSeconds = 97.f;
			LateZone->Tick(0.f);
			TestEqual(TEXT("A late receiver grows from the server's start, not its own BeginPlay"),
					  LateZone->GetTestRadius(), 350.f, 0.01f);
		}
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSummonedDummyTest, "GeoTrinity.Characters.SummonedDummy", GeoEncounterTestFlags)

bool FGeoSummonedDummyTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	TSoftClassPtr<AGeoEffectZone> const ProjectZoneClass = GetDefault<UGameDataSettings>()->DefaultZoneClass;
	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = AGeoTestReadableZone::StaticClass();
	AEnemyCharacter* const Dummy =
		Arena->SpawnAdd(AGeoTestSummonedDummy::StaticClass(), FTransform(FVector(300.f, 0.f, 0.f)));
	if (TestNotNull(TEXT("The dummy spawns"), Dummy))
	{
		TActorIterator<AGeoTestReadableZone> const Zone(World);
		if (TestTrue(TEXT("The default zone spawns at its feet"), static_cast<bool>(Zone)))
		{
			World->TimeSeconds += 5.f;
			Zone->Tick(0.f);
			TestEqual(TEXT("It grows by the dummy's ZoneParams"), Zone->GetTestRadius(), 350.f, 0.01f);
		}

		Dummy->Death();
		AEnemyCharacter* Ally = nullptr;
		for (TActorIterator<AEnemyCharacter> It(World); It && !Ally; ++It)
		{
			Ally = It->ActorHasTag(TEXT("Ally")) ? *It : nullptr;
		}

		if (TestNotNull(TEXT("Killed, it leaves its ally"), Ally))
		{
			TestTrue(TEXT("An add of the same arena"), Ally->Arena.Get() == Arena);
			TestEqual(TEXT("With the ally setup's team"), Ally->GetGenericTeamId(),
					  FGenericTeamId(static_cast<uint8>(ETeam::Player)));
			TestEqual(TEXT("In its place"), Ally->GetActorLocation().X, 300.0, 1.0);
		}
	}

	GetMutableDefault<UGameDataSettings>()->DefaultZoneClass = ProjectZoneClass;
	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSummonAddSetupTest, "GeoTrinity.Abilities.SummonAddSetup", GeoEncounterTestFlags)

bool FGeoSummonAddSetupTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Arena->GetBoss());
	ASC->CreatePatternInstance(UGeoTestBarePattern::StaticClass(), FGameplayTag());
	FGameplayAbilitySpecHandle const Handle =
		ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestSummonAddAbility::StaticClass(), 1));
	UGeoTestSummonAddAbility* const Summon =
		Cast<UGeoTestSummonAddAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	if (TestNotNull(TEXT("The summon is instanced"), Summon))
	{
		Summon->Summon(FVector2D(400.f, 0.f));
		AEnemyCharacter* Add = nullptr;
		for (TActorIterator<AEnemyCharacter> It(World); It && !Add; ++It)
		{
			Add = It->ActorHasTag(TEXT("Summoned")) ? *It : nullptr;
		}

		if (TestNotNull(TEXT("The add spawns with the summon's AddSetup"), Add))
		{
			TestTrue(TEXT("An add of the caster's arena"), Arena->GetLiveAdds().Contains(Add));
			TestEqual(TEXT("With the setup's max health"),
					  GeoASLib::GetGeoAscFromActor(Add)->GetNumericAttributeBase(
						  UGeoAttributeSetBase::GetMaxHealthAttribute()),
					  300.f);
			TestEqual(TEXT("On the summoned spot"), Add->GetActorLocation().X, 400.0, 1.0);
		}
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoArenaBossSetupTest, "GeoTrinity.Arena.BossSetup", GeoEncounterTestFlags)

bool FGeoArenaBossSetupTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	World->SetGameInstance(NewObject<UGameInstance>(GEngine));
	AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	FGameplayTag const ArenaTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	AGeoTargetPoint* const AddPoint = World->SpawnActor<AGeoTargetPoint>(FVector(500.f, 0.f, 0.f), FRotator());
	AddPoint->GameplayTags.AddTag(FGeoGameplayTags::Get().TargetPoint_AddSpawn);
	AddPoint->GameplayTags.AddTag(ArenaTag);

	FGameplayTag const AbilityTag = UGeoTestCastAbility::GetCastTag();
	AGeoTestCasterArena* const Arena =
		World->SpawnActorDeferred<AGeoTestCasterArena>(AGeoTestCasterArena::StaticClass(), FTransform::Identity);
	Arena->ArenaTag = ArenaTag;
	FGeoEnemySetup& BossSetup = Arena->EditBossSetup();
	BossSetup.Team = ETeam::Neutral;
	BossSetup.AbilityTags = {AbilityTag};
	BossSetup.MaxHealth = FScalableFloat(500.f);
	BossSetup.BehaviourStateTree = NewObject<UStateTree>(Arena);
	FGeoArenaAdd& StartAdd = Arena->EditStartAdds().AddDefaulted_GetRef();
	StartAdd.Class = AEnemyCharacter::StaticClass();
	StartAdd.Setup.Team = ETeam::Player;
	StartAdd.Setup.Tag = TEXT("StartAdd");
	Arena->FinishSpawning(FTransform::Identity);

	auto const TestBoss = [this, Arena, AbilityTag](TCHAR const* const When)
	{
		AEnemyCharacter* const Boss = Arena->GetBoss();
		if (TestNotNull(When, Boss))
		{
			UGeoAbilitySystemComponent const* const ASC = GeoASLib::GetGeoAscFromActor(Boss);
			TestEqual(TEXT("The setup's team"), Boss->GetGenericTeamId(),
					  FGenericTeamId(static_cast<uint8>(ETeam::Neutral)));
			TestTrue(TEXT("The setup's abilities"), ASC->StartupAbilityTags.Contains(AbilityTag));
			TestEqual(TEXT("The setup's max health"),
					  ASC->GetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute()), 500.f);
			TestTrue(TEXT("The setup's behaviour tree"),
					 Boss->BehaviourStateTree == Arena->EditBossSetup().BehaviourStateTree);
			TestTrue(TEXT("Still the arena's boss"), Boss->Arena.Get() == Arena);
		}

		TArray<AEnemyCharacter const*> const Adds = Arena->GetLiveAdds();
		if (TestEqual(TEXT("One start add"), Adds.Num(), 1))
		{
			TestTrue(TEXT("With its setup's tag"), Adds[0]->ActorHasTag(TEXT("StartAdd")));
			TestEqual(TEXT("With its setup's team"), Adds[0]->GetGenericTeamId(),
					  FGenericTeamId(static_cast<uint8>(ETeam::Player)));
			TestEqual(TEXT("On its add point"), Adds[0]->GetActorLocation().X, 500.0, 1.0);
		}
	};
	TestBoss(TEXT("The arena spawns its boss"));
	Arena->ResetBoss();
	TestBoss(TEXT("A reset spawns it again"));

	DestroyTestWorld(World);
	return true;
}

#endif
