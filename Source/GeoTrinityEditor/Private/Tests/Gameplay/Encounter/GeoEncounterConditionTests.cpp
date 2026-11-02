// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoCompareCondition.h"
#include "Encounter/GeoEncounter.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoEncounterConditionTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActionConditionTest, "GeoTrinity.Encounter.ActionCondition",
								 GeoEncounterConditionTestFlags)

bool FGeoEncounterActionConditionTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);

	UGeoTestAction* const Plain = Encounter->NewAction("Plain");
	UGeoTestAction* const Failing = Encounter->NewAction("Failing");
	UGeoTestAction* const Passing = Encounter->NewAction("Passing");
	UGeoTestAction* const Last = Encounter->NewAction("Last");
	Failing->Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 5.f);
	Passing->Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 0.f);
	Encounter->EditSteps()[0].OnEnter = {Plain, Failing, Passing, Last};

	UGeoTestAction* const OnRead = Encounter->NewAction("OnRead");
	UGeoTestAction* const OnMissed = Encounter->NewAction("OnMissed");
	UGeoTestAction* const AfterMissed = Encounter->NewAction("AfterMissed");
	OnRead->Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 3.f);
	OnMissed->Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 4.f);
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {OnRead, OnMissed, AfterMissed});

	Encounter->Start();
	TestEqual(TEXT("No condition: executed"), Plain->Runs, 1);
	TestEqual(TEXT("Failed condition: not executed"), Failing->Runs, 0);
	TestEqual(TEXT("Passing condition: executed"), Passing->Runs, 1);
	TestEqual(TEXT("The list goes on after a failed one and ends"), Last->Runs, 1);

	Trigger->FireNow(3);
	TestEqual(TEXT("In a rule's list the condition reads the trigger's payload"), OnRead->Runs, 1);
	TestEqual(TEXT("A failing payload compare skips the action"), OnMissed->Runs, 0);
	TestEqual(TEXT("The rule's list goes on"), AfterMissed->Runs, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActionConditionEndsTest, "GeoTrinity.Encounter.ActionConditionEnds",
								 GeoEncounterConditionTestFlags)

bool FGeoEncounterActionConditionEndsTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);

	UGeoTestLatentAction* const SkippedLatent = Encounter->NewAction<UGeoTestLatentAction>("SkippedLatent");
	UGeoTestAction* const AfterLatent = Encounter->NewAction("AfterLatent");
	SkippedLatent->Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 5.f);
	Encounter->EditSteps()[0].OnEnter = {SkippedLatent, AfterLatent};

	UGeoTestAction* const RanFirst = Encounter->NewAction("RanFirst");
	UGeoTestAction* const FailingLast = Encounter->NewAction("FailingLast");
	FailingLast->Condition = Encounter->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 5.f);
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {RanFirst, FailingLast}, false, true);

	Encounter->Start();
	TestEqual(TEXT("A failed latent action is never awaited: the list goes on with no Finish"), AfterLatent->Runs, 1);
	TestEqual(TEXT("A failed latent action never ran"), SkippedLatent->Runs, 0);

	Trigger->FireNow();
	TestEqual(TEXT("The list ran"), RanFirst->Runs, 1);
	TestEqual(TEXT("A failed last action still ends the list: the completing rule completes the step"),
			  Encounter->GetCurrentStepName(), FName("B"));

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActionConditionExitTest, "GeoTrinity.Encounter.ActionConditionExit",
								 GeoEncounterConditionTestFlags)

bool FGeoEncounterActionConditionExitTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoTestAction* const OnExit = Encounter->NewAction("OnExit");
	OnExit->Condition = Encounter->NewCompare(EGeoCompareSource::Counter, EGeoCompareOperator::Equal, 1.f, "Seen");
	Encounter->EditSteps()[0].OnExit = {OnExit};
	UGeoTestTrigger* const Rewind = Encounter->AddRule(0, {}, false, true, "A");

	Encounter->Start();
	Rewind->FireNow();
	TestEqual(TEXT("Counter not written: the exit action's condition fails"), OnExit->Runs, 0);

	Encounter->AddToEncounterCounter("Seen");
	Rewind->FireNow();
	TestEqual(TEXT("An OnExit condition reads the counters of the step being left"), OnExit->Runs, 1);
	TestEqual(TEXT("The counters are cleared on the re-entry"), Encounter->GetCounter("Seen"), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActionConditionValidationTest,
								 "GeoTrinity.Encounter.ActionConditionValidation", GeoEncounterConditionTestFlags)

bool FGeoEncounterActionConditionValidationTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	auto const ErrorsOf = [World](FName const CounterName)
	{
		AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
		Encounter->AddSteps(1);
		UGeoTestAction* const Action = Encounter->NewAction("Conditioned");
		Action->Condition =
			Encounter->NewCompare(EGeoCompareSource::Counter, EGeoCompareOperator::Equal, 1.f, CounterName);
		Encounter->EditSteps()[0].OnEnter = {Action};
		FDataValidationContext Context;
		Encounter->IsDataValid(Context);
		return Context.GetNumErrors();
	};

	TestEqual(TEXT("A nameless counter compare on an action is an error"), ErrorsOf(NAME_None), 1);
	TestEqual(TEXT("A named one is fine"), ErrorsOf("Seen"), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActionConditionSkipTest, "GeoTrinity.Encounter.ActionConditionSkip",
								 GeoEncounterConditionTestFlags)

bool FGeoEncounterActionConditionSkipTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const First = World->SpawnActor<AGeoTestEncounter>();
	AGeoTestEncounter* const Second = World->SpawnActor<AGeoTestEncounter>();
	First->SetStartOnBeginPlay(true);
	First->AddSteps(1);
	UGeoTestAction* const Plain = First->NewAction("Plain");
	UGeoTestAction* const Passing = First->NewAction("Passing");
	UGeoTestAction* const Failing = First->NewAction("Failing");
	Passing->Condition = First->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 0.f);
	Failing->Condition = First->NewCompare(EGeoCompareSource::PayloadCount, EGeoCompareOperator::Equal, 9.f);
	First->EditSteps()[0].OnEnter = {Plain, Passing, Failing};
	First->AddRule(0, {First->NewStartAction(Second)});

	Second->AddSteps(1);
	Second->AddRule(0, {});

	Second->StartAfterSkippingChain();
	TestEqual(TEXT("The plain action is skipped"), Plain->Skips, 1);
	TestEqual(TEXT("A conditioned action is never skipped (passing)"), Passing->Skips, 0);
	TestEqual(TEXT("A conditioned action is never skipped (failing)"), Failing->Skips, 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterStepVisitTest, "GeoTrinity.Encounter.StepVisit",
								 GeoEncounterConditionTestFlags)

bool FGeoEncounterStepVisitTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoTestTrigger* const Rewind = Encounter->AddRule(0, {}, false, true, "A");
	UGeoTestTrigger* const Leave = Encounter->AddRule(0, {}, false, true, "B");
	Encounter->AddRule(1, {});
	UGeoCompareCondition* const IsSecond =
		Encounter->NewCompare(EGeoCompareSource::StepVisit, EGeoCompareOperator::Equal, 2.f);

	TestEqual(TEXT("Idle: 0"), Encounter->GetStepVisit(), 0);
	Encounter->Start();
	TestEqual(TEXT("First entry: 1"), Encounter->GetStepVisit(), 1);
	TestFalse(TEXT("The StepVisit source reads it"), IsSecond->Evaluate(FGeoEncounterPayload()));

	Encounter->AddToEncounterCounter("Seen");
	Rewind->FireNow();
	TestEqual(TEXT("A step after itself: visit 2"), Encounter->GetStepVisit(), 2);
	TestEqual(TEXT("Counters are cleared on re-entry"), Encounter->GetCounter("Seen"), 0);
	TestTrue(TEXT("The StepVisit source passes on visit 2"), IsSecond->Evaluate(FGeoEncounterPayload()));

	Leave->FireNow();
	TestEqual(TEXT("Another step has its own visit"), Encounter->GetStepVisit(), 1);
	Encounter->GoToStep("A");
	TestEqual(TEXT("Back to the first step: the next number"), Encounter->GetStepVisit(), 3);

	Encounter->Restart();
	TestEqual(TEXT("Restart: back to 1"), Encounter->GetStepVisit(), 1);
	Rewind->FireNow();
	TestEqual(TEXT("Counted again"), Encounter->GetStepVisit(), 2);

	Encounter->Abort();
	TestEqual(TEXT("Aborted: Idle, 0"), Encounter->GetStepVisit(), 0);
	Encounter->Start();
	TestEqual(TEXT("Abort then Start: 1"), Encounter->GetStepVisit(), 1);

	DestroyTestWorld(World);
	return true;
}

#endif
