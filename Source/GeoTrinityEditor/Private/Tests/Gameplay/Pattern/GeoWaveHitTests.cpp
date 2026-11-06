// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/EnemyCharacter.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestHazardPattern.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoWaveHitTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

static AEnemyCharacter* PrepareWaveHitWorld(FAutomationTestBase& Test, UWorld& World)
{
	World.SetGameInstance(NewObject<UGameInstance>(GEngine));
	Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, -1);
	World.SetGameState(World.SpawnActor<AGeoGameState>());
	return World.SpawnActor<AGeoTestCasterArena>()->GetBoss();
}

/** The room of the Zones wave: the front leaves the near wall, the spots sit in the 1400 x 2200 room around it. */
static TArray<FVector2D> GetWaveRoomSpots()
{
	TArray<FVector2D> Spots;
	for (float const Along : {50.f, 400.f, 700.f, 1000.f, 1350.f})
	{
		for (float const Across : {-1050.f, -700.f, -350.f, 350.f, 700.f, 1050.f})
		{
			Spots.Add(FVector2D(Along, Across));
		}
	}

	return Spots;
}

/** True when some moment of the hazard, sampled like the judge (120 per second), hits Location. */
static bool IsHitDuringHazard(UGeoTestLateralWavePattern const& Pattern, FVector2D const Location)
{
	float const SampleInterval = 1.f / 120.f;
	for (float SpentTime = 0.f; SpentTime <= Pattern.GetDuration(); SpentTime += SampleInterval)
	{
		if (Pattern.IsHitAt(Location, SpentTime))
		{
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// The lethal wave of the Zones room, no safe spot: a pawn standing anywhere in the room is hit as the front passes,
// on the first firing of the pattern and on the next ones.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoWaveHitsEverySpotTest, "GeoTrinity.Pattern.WaveHits.EverySpot",
								 GeoWaveHitTestFlags)

bool FGeoWaveHitsEverySpotTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* const World = CreateTestWorld();
	AEnemyCharacter* const Boss = PrepareWaveHitWorld(*this, *World);
	UGeoTestLateralWavePattern* const Pattern =
		CastChecked<UGeoTestLateralWavePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
			UGeoTestLateralWavePattern::StaticClass(), FGameplayTag()));

	FVector2D const Origin(-700.f, 0.f);
	for (int32 Firing = 1; Firing <= 3; ++Firing)
	{
		LaunchTestPattern(*Pattern, Boss, {Origin});
		for (FVector2D const& Spot : GetWaveRoomSpots())
		{
			TestTrue(
				*FString::Printf(TEXT("Firing %d: the wave hits (%.0f, %.0f) from its origin"), Firing, Spot.X, Spot.Y),
				IsHitDuringHazard(*Pattern, Origin + Spot));
		}

		Pattern->StopFiring();
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// The safe-spot wave: the pawn is hit everywhere out of the lanes and in none of them, on every firing.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoWaveLanesTest, "GeoTrinity.Pattern.WaveHits.Lanes", GeoWaveHitTestFlags)

bool FGeoWaveLanesTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* const World = CreateTestWorld();
	AEnemyCharacter* const Boss = PrepareWaveHitWorld(*this, *World);
	UGeoTestLateralWavePattern* const Pattern =
		CastChecked<UGeoTestLateralWavePattern>(GeoASLib::GetGeoAscFromActor(Boss)->CreatePatternInstance(
			UGeoTestLateralWavePattern::StaticClass(), FGameplayTag()));

	// Lanes of the first safe-spot wave (X is irrelevant to a lane, Y the lane); SpotRadius 150 by default.
	TArray<float> const LaneY = {-650.f, 0.f, 650.f};
	FLateralWavePatternData WaveData;
	for (float const Y : LaneY)
	{
		WaveData.SafeSpotOffsets.Add(FVector2D(0.f, Y));
	}

	FVector2D const Origin(-700.f, 0.f);
	for (int32 Firing = 1; Firing <= 3; ++Firing)
	{
		LaunchTestPattern(*Pattern, Boss, {Origin}, GeoDifficultyMask::Original,
						  TInstancedStruct<FPatternData>::Make<FLateralWavePatternData>(WaveData));
		for (FVector2D const& Spot : GetWaveRoomSpots())
		{
			bool const bInLane = LaneY.ContainsByPredicate(
				[&Spot](float const Y)
				{
					return FMath::Abs(Spot.Y - Y) < 120.f;
				});
			bool const bOutOfEveryLane = !LaneY.ContainsByPredicate(
				[&Spot](float const Y)
				{
					return FMath::Abs(Spot.Y - Y) < 180.f;
				});
			if (bInLane)
			{
				TestFalse(*FString::Printf(TEXT("Firing %d: (%.0f, %.0f) in a lane is safe"), Firing, Spot.X, Spot.Y),
						  IsHitDuringHazard(*Pattern, Origin + Spot));
			}
			else if (bOutOfEveryLane)
			{
				TestTrue(
					*FString::Printf(TEXT("Firing %d: (%.0f, %.0f) out of the lanes is hit"), Firing, Spot.X, Spot.Y),
					IsHitDuringHazard(*Pattern, Origin + Spot));
			}
		}

		Pattern->StopFiring();
	}

	DestroyTestWorld(World);
	return true;
}

#endif
