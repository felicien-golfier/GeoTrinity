// Copyright 2024 GeoTrinity. All Rights Reserved.

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
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Encounter/Action/Cat/GeoCatMontageAction.h"
#include "Encounter/Action/Cat/GeoCatRoamAction.h"
#include "Encounter/Action/Cat/GeoCatWalkAction.h"
#include "Encounter/Action/GeoActivateAbilityAction.h"
#include "Encounter/Action/GeoAddToCounterAction.h"
#include "Encounter/Action/GeoCatLineAction.h"
#include "Encounter/Action/GeoSetActorHiddenAction.h"
#include "Encounter/Action/GeoSetMovementLockedAction.h"
#include "Encounter/Action/GeoSetTeleporterOpenAction.h"
#include "Encounter/Action/GeoSpawnEnemiesAction.h"
#include "Encounter/Action/GeoSpawnZoneAction.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Encounter/Trigger/GeoAbilityActivatedTrigger.h"
#include "Encounter/Trigger/GeoActorDiedTrigger.h"
#include "Encounter/Trigger/GeoCounterTrigger.h"
#include "Encounter/Trigger/GeoEventTrigger.h"
#include "Encounter/Trigger/GeoHealthThresholdTrigger.h"
#include "Encounter/Trigger/GeoPlayerIdleTrigger.h"
#include "Encounter/Trigger/GeoTimerTrigger.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameplayTagsManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Settings/GameDataSettings.h"
#include "Tests/GeoTestDeployable.h"
#include "Tests/GeoTestEncounter.h"
#include "Tests/GeoTestPatternAbility.h"
#include "Tests/GeoTestWorld.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

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

/** A caster arena in World, its hidden caster spawned. */
static AGeoTestCasterArena* SpawnCasterArena(FAutomationTestBase& Test, UWorld& World)
{
	World.SetGameInstance(NewObject<UGameInstance>(GEngine));
	Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
	World.SetGameState(World.SpawnActor<AGeoGameState>());
	return World.SpawnActor<AGeoTestCasterArena>();
}

/** Grants UGeoTestCastAbility to the arena's hidden caster. */
static FGameplayAbilitySpecHandle GrantCastAbility(AGeoArena const& Arena)
{
	return GeoASLib::GetGeoAscFromActor(Arena.GetBoss())
		->GiveAbility(FGameplayAbilitySpec(UGeoTestCastAbility::StaticClass(), 1));
}

static UGeoActivateAbilityAction* NewActivateAction(AGeoTestEncounter& Encounter, AGeoArena* Arena,
													bool const bWaitForEnd = true)
{
	UGeoActivateAbilityAction* const Action = NewObject<UGeoActivateAbilityAction>(&Encounter);
	Action->Arena = Arena;
	Action->AbilityTag = UGeoTestCastAbility::GetCastTag();
	Action->bWaitForEnd = bWaitForEnd;
	return Action;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoHiddenCasterTest, "GeoTrinity.Characters.HiddenCaster", GeoEncounterTestFlags)

bool FGeoHiddenCasterTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	AEnemyCharacter const* const Caster = Arena->GetBoss();
	if (TestTrue(TEXT("The arena spawns the caster"), Caster && Caster->IsA<AHiddenCasterCharacter>()))
	{
		TestFalse(TEXT("Nothing can damage it"), Caster->CanBeDamaged());
		TestFalse(TEXT("Nothing can touch it"), Caster->GetActorEnableCollision());
		TestTrue(TEXT("Nothing is drawn"), Caster->IsHidden());
		TestNull(TEXT("No health bar"), Caster->FindComponentByClass<UWidgetComponent>());
		TestFalse(TEXT("Never aggroed"), Caster->CanBeAggroed());
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
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	FGameplayAbilitySpecHandle const Handle = GrantCastAbility(*Arena);
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Arena->GetBoss());

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const AfterWait = Encounter->NewAction("AfterWait");
	UGeoTestTrigger* const WaitTrigger = Encounter->AddRule(0, {NewActivateAction(*Encounter, Arena), AfterWait});
	UGeoTestAction* const AfterNoWait = Encounter->NewAction("AfterNoWait");
	UGeoTestTrigger* const NoWaitTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, Arena, false), AfterNoWait});
	Encounter->Start();

	WaitTrigger->FireNow();
	TestTrue(TEXT("The ability runs on the arena's boss"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
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
	AddExpectedMessage(TEXT("no ability tagged"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2,
					   false);
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const AfterUnknownTag = Encounter->NewAction("AfterUnknownTag");
	UGeoTestTrigger* const UnknownTagTrigger =
		Encounter->AddRule(0, {NewActivateAction(*Encounter, Arena), AfterUnknownTag});
	UGeoTestAction* const AfterNoBoss = Encounter->NewAction("AfterNoBoss");
	UGeoTestTrigger* const NoBossTrigger = Encounter->AddRule(0, {NewActivateAction(*Encounter, nullptr), AfterNoBoss});
	Encounter->Start();

	UnknownTagTrigger->FireNow();
	TestEqual(TEXT("A boss with no such ability: the step goes on"), AfterUnknownTag->Runs, 1);
	NoBossTrigger->FireNow();
	TestEqual(TEXT("No boss: the step goes on"), AfterNoBoss->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityCancelTest, "GeoTrinity.Encounter.ActivateAbilityCancel",
								 GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityCancelTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = SpawnCasterArena(*this, *World);
	FGameplayAbilitySpecHandle const Handle = GrantCastAbility(*Arena);
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Arena->GetBoss());

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const CastTrigger = Encounter->AddRule(0, {NewActivateAction(*Encounter, Arena), After});
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	CastTrigger->FireNow();
	TestTrue(TEXT("The ability runs"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	LeaveTrigger->FireNow();
	TestFalse(TEXT("Leaving the step cancels the ability"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
	TestEqual(TEXT("The cancelled wait never reaches the next action"), After->Runs, 0);

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterTimerTest, "GeoTrinity.Encounter.Timer", GeoEncounterTestFlags)

bool FGeoEncounterTimerTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTimerTrigger* const Trigger = NewObject<UGeoTimerTrigger>(Encounter);
	Trigger->Delay = 1.f;
	Trigger->bLooping = true;
	UGeoTestAction* const Reaction = Encounter->NewAction("Reaction");
	Encounter->AddRuleWithTrigger(0, Trigger, {Reaction}, true);
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 0.5f);
	TestEqual(TEXT("Not before Delay"), Reaction->Runs, 0);
	TickTestTimers(*World, 0.51f);
	TestEqual(TEXT("Fires after Delay"), Reaction->Runs, 1);
	TestEqual(TEXT("Count = firings so far"), Reaction->LastPayload.Count, 1);
	TickTestTimers(*World, 1.01f);
	TestEqual(TEXT("Looping: fires every Delay"), Reaction->LastPayload.Count, 2);
	LeaveTrigger->FireNow();
	TickTestTimers(*World, 1.01f);
	TestEqual(TEXT("Disarmed: the timer is gone"), Reaction->Runs, 2);

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
	Counting->Begin(FGeoEncounterPayload());
	TestEqual(TEXT("A counter token reads its encounter's counter, an unset one is 0, other tokens stay"),
			  HUD->Lines.Last(), FString("That's 3 deaths, 3. 0 {Key:X}"));

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
	AGeoTestGrowingZone* const Zone = Cast<AGeoTestGrowingZone>(
		GeoASLib::FullySpawnDeployable(AGeoTestGrowingZone::StaticClass(), Data, FTransform::Identity, World));
	if (TestNotNull(TEXT("The zone spawns"), Zone))
	{
		TestEqual(TEXT("It starts at its spawn size"), Zone->GetTestRadius(), 100.f);
		World->TimeSeconds += 5.f;
		Zone->Tick(0.f);
		TestEqual(TEXT("Half of GrowthSeconds: half way to MaxRadius"), Zone->GetTestRadius(), 350.f, 0.01f);
		TestEqual(TEXT("The capsule follows"),
				  Zone->FindComponentByClass<UCapsuleComponent>()->GetUnscaledCapsuleRadius(), 350.f, 0.01f);
		TestEqual(TEXT("The replicated spawn size never changes"), Zone->GetSpawnSize(), 100.f);
		World->TimeSeconds += 60.f;
		Zone->Tick(0.f);
		TestEqual(TEXT("It stops at MaxRadius"), Zone->GetTestRadius(), 600.f, 0.01f);
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
	AEnemyCharacter* const Dummy =
		Arena->SpawnAdd(AGeoTestSummonedDummy::StaticClass(), FTransform(FVector(300.f, 0.f, 0.f)));
	if (TestNotNull(TEXT("The dummy spawns"), Dummy))
	{
		TestTrue(TEXT("A zone grows at its feet"), static_cast<bool>(TActorIterator<AGeoTestGrowingZone>(World)));
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

	DestroyTestWorld(World);
	return true;
}

#endif
