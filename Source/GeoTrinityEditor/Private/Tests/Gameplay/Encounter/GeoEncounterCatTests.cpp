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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterTimedCatLineTest, "GeoTrinity.Encounter.TimedCatLine",
								 GeoEncounterTestFlags)

bool FGeoEncounterTimedCatLineTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	AGeoTestHUD* const HUD = World->SpawnActor<AGeoTestHUD>();
	PlayerController->MyHUD = HUD;

	AGeoTestEncounter* const Encounter = World->SpawnActor<AGeoTestEncounter>();
	Encounter->AddSteps(1);
	UGeoCatLineAction* const Timed = NewObject<UGeoCatLineAction>(Encounter);
	Timed->Line = INVTEXT("Hi");
	Timed->bTimed = true;
	Timed->Seconds = FScalableFloat(5.f);
	UGeoTestAction* const After = Encounter->NewAction("After");
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {Timed, After}, true);
	Encounter->Start();

	Trigger->FireNow();
	TestEqual(TEXT("A timed line asks the HUD for its Seconds"), HUD->LineSeconds, TArray<float>{5.f});
	TestEqual(TEXT("The run goes on as the timed line starts"), After->Runs, 1);

	Timed->Line = INVTEXT("A line far too long to type out in a single second of the Cat's time");
	Timed->Seconds = FScalableFloat(1.f);
	FDataValidationContext Context;
	Timed->IsDataValid(Context);
	TestTrue(TEXT("A text that cannot type within Seconds is a data error"), Context.GetNumErrors() > 0);

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

#endif
