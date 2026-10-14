// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "Misc/AutomationTest.h"
#include "System/Stats/GeoCombatStatsSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoCombatStatsTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCombatStatsFreezeOnDeathTest, "GeoTrinity.CombatStats.FreezeOnDeath",
								 GeoCombatStatsTestFlags)

bool FGeoCombatStatsFreezeOnDeathTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->SetGameState(World->SpawnActor<AGeoGameState>());
	AGeoPlayerState* Player = World->SpawnActor<AGeoPlayerState>();
	UGeoCombatStatsSubsystem* Stats = World->GetSubsystem<UGeoCombatStatsSubsystem>();

	if (TestNotNull(TEXT("Combat stats subsystem exists"), Stats))
	{
		// The world clock stays at 0: the fight starts there, and the explicit times below are the "now".
		Stats->ReportDamageDealt(Player, 300.f);

		Stats->ComputePlayerStats(5.f);
		TestEqual(TEXT("Alive: average over the time so far"), Player->GetFightDPS(), 60.f);

		Stats->ReportPlayerDied(Player, 10.f);
		Stats->ComputePlayerStats(10.f);
		TestEqual(TEXT("At death: average over the alive time"), Player->GetFightDPS(), 30.f);

		Stats->ComputePlayerStats(30.f);
		TestEqual(TEXT("Long after death: average unchanged"), Player->GetFightDPS(), 30.f);
		TestTrue(TEXT("Long after death: live rate decayed away"), Player->GetLiveDPS() < 1.f);
		TestEqual(TEXT("Totals untouched"), Player->GetTotalDamageDealt(), 300.f);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
