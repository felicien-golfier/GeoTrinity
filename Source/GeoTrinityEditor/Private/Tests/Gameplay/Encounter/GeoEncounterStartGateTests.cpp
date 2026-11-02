// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameFramework/DefaultPawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterStartGateTest, "GeoTrinity.Encounter.StartGate",
								 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeoEncounterStartGateTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter =
		World->SpawnActorDeferred<AGeoTestEncounter>(AGeoTestEncounter::StaticClass(), FTransform::Identity);
	Encounter->SetStartOnBeginPlay(true);
	Encounter->AddSteps(1);
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {});
	Encounter->FinishSpawning(FTransform::Identity);
	TestEqual(TEXT("No player controller: the encounter waits"), Trigger->ArmCount, 0);

	AGameModeBase* const GameMode = World->SpawnActor<AGameModeBase>();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	FGameModeEvents::OnGameModePostLoginEvent().Broadcast(GameMode, PlayerController);
	TestEqual(TEXT("A controller without a pawn: still waiting"), Trigger->ArmCount, 0);

	PlayerController->Possess(World->SpawnActor<ADefaultPawn>());
	TestEqual(TEXT("The possession starts it, once"), Trigger->ArmCount, 1);

	PlayerController->UnPossess();
	PlayerController->Possess(World->SpawnActor<ADefaultPawn>());
	TestEqual(TEXT("A later possession starts nothing"), Trigger->ArmCount, 1);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoEncounterStartGateEndPlayTest, "GeoTrinity.Encounter.StartGateEndPlay",
								 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeoEncounterStartGateEndPlayTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestEncounter* const Encounter =
		World->SpawnActorDeferred<AGeoTestEncounter>(AGeoTestEncounter::StaticClass(), FTransform::Identity);
	Encounter->SetStartOnBeginPlay(true);
	Encounter->AddSteps(1);
	UGeoTestTrigger* const Trigger = Encounter->AddRule(0, {});
	Encounter->FinishSpawning(FTransform::Identity);

	Encounter->Destroy();
	AGameModeBase* const GameMode = World->SpawnActor<AGameModeBase>();
	APlayerController* const PlayerController = World->SpawnActor<APlayerController>();
	FGameModeEvents::OnGameModePostLoginEvent().Broadcast(GameMode, PlayerController);
	PlayerController->Possess(World->SpawnActor<ADefaultPawn>());
	TestEqual(TEXT("An encounter that ended play never starts"), Trigger->ArmCount, 0);

	DestroyTestWorld(World);
	return true;
}

#endif
