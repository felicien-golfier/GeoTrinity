// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArenaVolume.h"
#include "Characters/EnemyCharacter.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestHazardPattern.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoClientSafeSpotTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

namespace
{
	/** A match world with its arena and boss, set up as the server does; Net is the machine the pattern then runs on.
	 */
	AEnemyCharacter* PrepareHazardWorld(FAutomationTestBase& Test, UWorld& World)
	{
		World.SetGameInstance(NewObject<UGameInstance>(GEngine));
		Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, -1);
		World.SetGameState(World.SpawnActor<AGeoGameState>());
		// A test world has no local player controller: a client reading its ping-corrected server time says so.
		Test.AddExpectedError(TEXT("No local player controller found"), EAutomationExpectedErrorFlags::Contains, 0);
		return World.SpawnActor<AGeoTestCasterArena>()->GetBoss();
	}
} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// What a player sees is what the pattern uploads to the safe spot mask: one slot per spot, on every machine.
// The listen server is the control; the client must show the same.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoClientSafeSpotFixedTest, "GeoTrinity.Pattern.ClientSafeSpots.LateralFixed",
								 GeoClientSafeSpotTestFlags)

bool FGeoClientSafeSpotFixedTest::RunTest(FString const& /*Parameters*/)
{
	for (ENetMode const NetMode : {NM_ListenServer, NM_Client})
	{
		FString const Machine = NetMode == NM_Client ? TEXT("Client") : TEXT("Listen server");
		UWorld* const World = CreateTestWorld();
		AEnemyCharacter* const Boss = PrepareHazardWorld(*this, *World);
		UGeoTestLateralWavePattern* const Pattern =
			CastChecked<UGeoTestLateralWavePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
				UGeoTestLateralWavePattern::StaticClass(), FGameplayTag()));

		FLateralWavePatternData WaveData;
		WaveData.SafeSpotOffsets = {FVector2D(500.f, 300.f), FVector2D(500.f, -300.f)};
		SetTestNetMode(*World, NetMode);
		TestEqual(*FString::Printf(TEXT("%s: the world answers as that machine"), *Machine), World->GetNetMode(),
				  NetMode);
		LaunchTestPattern(*Pattern, Boss, {FVector2D::ZeroVector}, GeoDifficultyMask::Original,
						  TInstancedStruct<FPatternData>::Make<FLateralWavePatternData>(WaveData));

		TestEqual(*FString::Printf(TEXT("%s: one mask slot per fixed spot"), *Machine),
				  GeoTestMask::CountUsedSlots(*World, *Pattern->GetMask()), 2);

		SetTestNetMode(*World, NM_ListenServer);
		DestroyTestWorld(World);
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoClientSafeSpotAddsTest, "GeoTrinity.Pattern.ClientSafeSpots.LateralAdds",
								 GeoClientSafeSpotTestFlags)

bool FGeoClientSafeSpotAddsTest::RunTest(FString const& /*Parameters*/)
{
	for (ENetMode const NetMode : {NM_ListenServer, NM_Client})
	{
		FString const Machine = NetMode == NM_Client ? TEXT("Client") : TEXT("Listen server");
		UWorld* const World = CreateTestWorld();
		AEnemyCharacter* const Boss = PrepareHazardWorld(*this, *World);
		AGeoArena* const Arena = AGeoArena::GetArenaOfBoss(Boss);
		World->SpawnActor<AGeoArenaVolume>(FVector::ZeroVector, FRotator::ZeroRotator);
		// Not through SpawnAdd: the arena's list is server-only, a client has none.
		AEnemyCharacter* const Add = AEnemyCharacter::SpawnEnemy(*World, AEnemyCharacter::StaticClass(),
																 FVector(300.f, 200.f, 0.f), Arena, nullptr, nullptr);
		if (!TestNotNull(TEXT("The add spawns"), Add))
		{
			DestroyTestWorld(World);
			return true;
		}

		UGeoTestLateralWavePattern* const Pattern =
			CastChecked<UGeoTestLateralWavePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
				UGeoTestLateralWavePattern::StaticClass(), FGameplayTag()));
		SetTestNetMode(*World, NetMode);
		LaunchTestPattern(*Pattern, Boss, {FVector2D::ZeroVector});

		TestEqual(*FString::Printf(TEXT("%s: the live add's lane is in the mask"), *Machine),
				  GeoTestMask::CountUsedSlots(*World, *Pattern->GetMask()), 1);

		SetTestNetMode(*World, NM_ListenServer);
		DestroyTestWorld(World);
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoClientSafeSpotPillarsTest, "GeoTrinity.Pattern.ClientSafeSpots.DevastatingPillars",
								 GeoClientSafeSpotTestFlags)

bool FGeoClientSafeSpotPillarsTest::RunTest(FString const& /*Parameters*/)
{
	for (ENetMode const NetMode : {NM_ListenServer, NM_Client})
	{
		FString const Machine = NetMode == NM_Client ? TEXT("Client") : TEXT("Listen server");
		UWorld* const World = CreateTestWorld();
		AEnemyCharacter* const Boss = PrepareHazardWorld(*this, *World);
		UPattern* const PillarPattern = GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
			UGeoTestPillarPattern::StaticClass(), FGameplayTag());
		LaunchTestPattern(*PillarPattern, Boss, {FVector2D(1000.f, 600.f)});
		PillarPattern->EndPattern();

		UGeoTestDevastatingWavePattern* const Pattern =
			CastChecked<UGeoTestDevastatingWavePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
				UGeoTestDevastatingWavePattern::StaticClass(), FGameplayTag()));
		SetTestNetMode(*World, NetMode);
		LaunchTestPattern(*Pattern, Boss, {FVector2D::ZeroVector});

		TestEqual(*FString::Printf(TEXT("%s: the pillar's shadow is in the mask"), *Machine),
				  GeoTestMask::CountUsedSlots(*World, *Pattern->GetMask()), 1);

		SetTestNetMode(*World, NM_ListenServer);
		DestroyTestWorld(World);
	}

	return true;
}

#endif
