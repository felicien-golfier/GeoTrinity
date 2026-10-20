// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/GeoTestEnemy.h"
#include "Tests/GeoTestWorld.h"
#include "World/GeoWorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

constexpr EAutomationTestFlags GeoVoidDeathTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** A begun test world with a GameState, an enemy whose body stays after death, and a count of OnCharacterDied. */
struct FGeoVoidDeathFixture
{
	FGeoVoidDeathFixture(FAutomationTestBase& Test)
	{
		World = CreateTestWorld();
		World->SetGameInstance(NewObject<UGameInstance>(GEngine));
		Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, 0);
		AGeoGameState* const GameState = World->SpawnActor<AGeoGameState>();
		World->SetGameState(GameState);
		GameState->OnCharacterDied.AddLambda(
			[this](AGeoCharacter& /*Character*/)
			{
				++DeathCount;
			});
		Enemy = World->SpawnActor<AGeoTestBodyEnemy>();
	}

	~FGeoVoidDeathFixture() { DestroyTestWorld(World); }

	void MoveUnderKillZ() const
	{
		Enemy->SetActorLocation(FVector(0.f, 0.f, World->GetWorldSettings()->KillZ - 100.f));
	}

	UWorld* World = nullptr;
	AGeoTestBodyEnemy* Enemy = nullptr;
	int32 DeathCount = 0;
};

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoVoidDeathTest, "GeoTrinity.Characters.VoidDeath", GeoVoidDeathTestFlags)

bool FGeoVoidDeathTest::RunTest(FString const& /*Parameters*/)
{
	FGeoVoidDeathFixture Fixture(*this);
	if (TestNotNull(TEXT("The enemy spawns"), Fixture.Enemy))
	{
		TestEqual(TEXT("The world's KillZ defaults to GeoKillZ"), Fixture.World->GetWorldSettings()->KillZ, GeoKillZ);
		Fixture.MoveUnderKillZ();
		Fixture.Enemy->CheckStillInWorld();
		TestTrue(TEXT("Under KillZ the character dies"), Fixture.Enemy->IsDead());
		TestTrue(TEXT("The engine's destroy is not run"), IsValid(Fixture.Enemy));
		TestEqual(TEXT("OnCharacterDied is broadcast once"), Fixture.DeathCount, 1);
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoVoidDeathWhileDeadTest, "GeoTrinity.Characters.VoidDeathWhileDead",
								 GeoVoidDeathTestFlags)

bool FGeoVoidDeathWhileDeadTest::RunTest(FString const& /*Parameters*/)
{
	FGeoVoidDeathFixture Fixture(*this);
	if (TestNotNull(TEXT("The enemy spawns"), Fixture.Enemy))
	{
		Fixture.MoveUnderKillZ();
		Fixture.Enemy->CheckStillInWorld();
		Fixture.Enemy->CheckStillInWorld();
		TestEqual(TEXT("A second fall while dead broadcasts nothing"), Fixture.DeathCount, 1);
		TestTrue(TEXT("The body is still there"), IsValid(Fixture.Enemy));
	}

	return true;
}

#endif
