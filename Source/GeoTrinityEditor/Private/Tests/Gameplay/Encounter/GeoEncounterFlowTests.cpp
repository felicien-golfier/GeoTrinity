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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterEnterDelayTest, "GeoTrinity.Encounter.EnterDelay", GeoEncounterTestFlags)

bool FGeoEncounterEnterDelayTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoDelayAction* const Delay = NewObject<UGeoDelayAction>(Encounter);
	Delay->Duration = FScalableFloat(1.f);
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

#endif
