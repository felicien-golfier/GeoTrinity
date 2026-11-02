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

#endif
