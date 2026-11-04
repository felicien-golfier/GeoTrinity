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
#include "Encounter/Action/GeoMenuGuideAction.h"
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
#include "Tests/Gameplay/GeoTestFlags.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityHeldTest, "GeoTrinity.Encounter.ActivateAbilityHeld",
								 GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityHeldTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	PrepareEnemyWorld(*this, *World);
	FGameplayAbilitySpecHandle Handle;
	UGeoAbilitySystemComponent* const ASC = GrantShared(*World, UGeoTestCastAbility::StaticClass(), Handle);
	int32 Ends = 0;
	ASC->OnAbilityEnded.AddLambda(
		[&Ends](FAbilityEndedData const&)
		{
			++Ends;
		});
	auto const IsRunning = [ASC, Handle]()
	{
		return ASC->FindAbilitySpecFromHandle(Handle)->IsActive();
	};

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoActivateAbilityAction* const Held = NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass(), false);
	Held->bHeldUntilStepLeft = true;
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const HoldTrigger = Encounter->AddRule(0, {Held, After}, true);
	UGeoTestTrigger* const LeaveTrigger = Encounter->AddRule(0, {}, false, true);
	UGeoTestTrigger* const RewindTrigger = Encounter->AddRule(0, {}, true, true, "A");
	Encounter->Start();

	HoldTrigger->FireNow();
	TestTrue(TEXT("The ability runs"), IsRunning());
	TestEqual(TEXT("The action finishes at once: the next one runs"), After->Runs, 1);
	TestTrue(TEXT("Still running once the action finished"), IsRunning());
	LeaveTrigger->FireNow();
	TestFalse(TEXT("Leaving the step cancels the ability"), IsRunning());

	Encounter->Restart();
	HoldTrigger->FireNow();
	TestTrue(TEXT("Fired again after a restart"), IsRunning());
	RewindTrigger->FireNow();
	TestFalse(TEXT("A rewind of the step cancels the ability"), IsRunning());

	Ends = 0;
	HoldTrigger->FireNow();
	HoldTrigger->FireNow();
	TestEqual(TEXT("Run again while held: the previous firing was cancelled"), Ends, 1);
	TestTrue(TEXT("Run again while held: a new firing runs"), IsRunning());

	Encounter->Abort();
	TestFalse(TEXT("An abort cancels the ability"), IsRunning());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterActivateAbilityHeldValidationTest,
								 "GeoTrinity.Encounter.ActivateAbilityHeldValidation", GeoEncounterTestFlags)

bool FGeoEncounterActivateAbilityHeldValidationTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();

	UGeoActivateAbilityAction* const Action = NewActivateAction(*Encounter, UGeoTestCastAbility::StaticClass(), false);
	Action->bHeldUntilStepLeft = true;
	FDataValidationContext NoWaitContext;
	Action->IsDataValid(NoWaitContext);
	TestEqual(TEXT("Held without waiting is valid"), NoWaitContext.GetNumErrors(), 0u);

	Action->bWaitForEnd = true;
	FDataValidationContext WaitContext;
	Action->IsDataValid(WaitContext);
	TestEqual(TEXT("Held and waiting for the end is flagged"), WaitContext.GetNumErrors(), 1u);

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
	if (TestNotNull(TEXT("The ability runs"), Ability)
		&& TestEqual(TEXT("One target location"), Ability->LaunchedLocations.Num(), 1))
	{
		TestEqual(TEXT("It targets its point in the action's room, not another room's (X)"),
				  Ability->LaunchedLocations[0].X, 400.0);
		TestEqual(TEXT("(Y)"), Ability->LaunchedLocations[0].Y, 250.0);
	}

	DestroyTestWorld(World);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterMenuGuideTest, "GeoTrinity.Encounter.MenuGuide", GeoEncounterTestFlags)

bool FGeoEncounterMenuGuideTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestHUD* const HUD = World->SpawnActor<AGeoTestHUD>();
	PlayerController->MyHUD = HUD;

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(2);
	UGeoMenuGuideAction* const Guide = NewObject<UGeoMenuGuideAction>(Encounter);
	Guide->Goal = EGeoMenuGuideGoal::ForgeChip;
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Guide}, false);
	UGeoTestTrigger* const Leave = Encounter->AddRule(0, {}, false, true);
	Encounter->Start();

	Trigger->FireNow();
	TestEqual(TEXT("The guide starts towards its goal"), HUD->GuideGoal, EGeoMenuGuideGoal::ForgeChip);

	Leave->FireNow();
	TestEqual(TEXT("Leaving the step stops the guide"), HUD->GuideGoal, EGeoMenuGuideGoal::None);

	DestroyTestWorld(World);
	return true;
}

#endif
