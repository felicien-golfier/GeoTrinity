// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Arena/GeoArenaVolume.h"
#include "Actor/GeoCat.h"
#include "Components/BoxComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

constexpr EAutomationTestFlags GeoCatTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** A bare cat (no mesh, no montage) in a begun world, with the ends of its orders recorded. */
struct FGeoCatFixture
{
	FGeoCatFixture()
	{
		World = CreateTestWorld();
		Cat = World->SpawnActor<AGeoCat>(FVector::ZeroVector, FRotator::ZeroRotator);
		Cat->OnOrderEnded.AddLambda(
			[this](bool const bCompleted)
			{
				Ends.Add(bCompleted);
			});
	}

	~FGeoCatFixture() { DestroyTestWorld(World); }

	/** A room of half size 400 x 300 centred on (500, -300), roamed with a 150 margin. */
	FGeoCatRoam MakeRoam() const
	{
		AGeoArenaVolume* const Room = World->SpawnActor<AGeoArenaVolume>(FVector(500.f, -300.f, 0.f), FRotator());
		Room->FindComponentByClass<UBoxComponent>()->SetBoxExtent(FVector(400.f, 300.f, 100.f));
		FGeoCatRoam Roam;
		Roam.Area = Room;
		Roam.Margin = 150.f;
		return Roam;
	}

	UWorld* World = nullptr;
	AGeoCat* Cat = nullptr;
	TArray<bool> Ends;
};

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatWalkTest, "GeoTrinity.Cat.Walk", GeoCatTestFlags)

bool FGeoCatWalkTest::RunTest(FString const& /*Parameters*/)
{
	FGeoCatFixture Fixture;
	AGeoCat* const Cat = Fixture.Cat;
	Cat->Walk({FVector(200.f, 0.f, 0.f), FVector(200.f, 200.f, 0.f)}, 100.f, 0.f);

	Cat->AdvanceMove(1.f);
	TestEqual(TEXT("Walks toward the first point at the order's speed"), Cat->GetActorLocation().X, 100.0);
	Cat->AdvanceMove(1.f);
	TestTrue(TEXT("Reaches the first point"), Cat->GetActorLocation().Equals(FVector(200.f, 0.f, 0.f)));
	TestTrue(TEXT("Not ended before the last point"), Fixture.Ends.IsEmpty());
	Cat->AdvanceMove(1.f);
	TestTrue(TEXT("Then heads to the second"), Cat->GetActorLocation().Equals(FVector(200.f, 100.f, 0.f)));
	Cat->AdvanceMove(1.f);
	TestTrue(TEXT("Reaches the last point"), Cat->GetActorLocation().Equals(FVector(200.f, 200.f, 0.f)));
	TestTrue(TEXT("Ends completed, once"), Fixture.Ends == TArray<bool>{true});
	Cat->AdvanceMove(1.f);
	TestTrue(TEXT("Stands still after"), Cat->GetActorLocation().Equals(FVector(200.f, 200.f, 0.f)));
	TestEqual(TEXT("No second end"), Fixture.Ends.Num(), 1);

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatWalkStopDistanceTest, "GeoTrinity.Cat.WalkStopDistance", GeoCatTestFlags)

bool FGeoCatWalkStopDistanceTest::RunTest(FString const& /*Parameters*/)
{
	FGeoCatFixture Fixture;
	AGeoCat* const Cat = Fixture.Cat;
	Cat->Walk({FVector(0.f, 500.f, 0.f)}, 150.f, 110.f);
	Cat->AdvanceMove(100.f);
	TestTrue(TEXT("Stops StopDistance before the last point"),
			 Cat->GetActorLocation().Equals(FVector(0.f, 390.f, 0.f)));
	TestEqual(TEXT("Facing it"), Cat->GetActorRotation().Yaw, 90.0, 0.01);
	TestTrue(TEXT("Completed"), Fixture.Ends == TArray<bool>{true});

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatReplaceTest, "GeoTrinity.Cat.Replace", GeoCatTestFlags)

bool FGeoCatReplaceTest::RunTest(FString const& /*Parameters*/)
{
	FGeoCatFixture Fixture;
	AGeoCat* const Cat = Fixture.Cat;
	Cat->Walk({FVector(1000.f, 0.f, 0.f)}, 150.f, 0.f);
	Cat->AdvanceMove(0.1f);
	Cat->Roam(Fixture.MakeRoam());
	TestTrue(TEXT("A new order ends the walk as replaced"), Fixture.Ends == TArray<bool>{false});
	TestTrue(TEXT("The new order runs"), Cat->IsOrderRunning());

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatRoamStaysInsideTest, "GeoTrinity.Cat.RoamStaysInside", GeoCatTestFlags)

bool FGeoCatRoamStaysInsideTest::RunTest(FString const& /*Parameters*/)
{
	FGeoCatFixture Fixture;
	AGeoCat* const Cat = Fixture.Cat;
	FGeoCatRoam const Roam = Fixture.MakeRoam();

	bool bAllInside = true;
	for (int32 Walk = 0; Walk < 50; ++Walk)
	{
		Cat->Roam(Roam);
		Cat->AdvanceMove(100.f);
		FVector const Location = Cat->GetActorLocation();
		bAllInside &= FMath::Abs(Location.X - 500.f) <= 250.f && FMath::Abs(Location.Y + 300.f) <= 150.f;
	}

	TestTrue(TEXT("Every roam spot is inside the room minus the margin"), bAllInside);
	TestTrue(TEXT("Roam never ends on its own"), Fixture.Ends.Num() == 49 && !Fixture.Ends.Contains(true));

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatMontageTest, "GeoTrinity.Cat.Montage", GeoCatTestFlags)

bool FGeoCatMontageTest::RunTest(FString const& /*Parameters*/)
{
	FGeoCatFixture Fixture;
	AGeoCat* const Cat = Fixture.Cat;
	Cat->Walk({FVector(1000.f, 0.f, 0.f)}, 150.f, 0.f);
	Cat->PlayMontage(nullptr);
	TestTrue(TEXT("Replaces the walk, then no montage: completes at once"), Fixture.Ends == TArray<bool>{false, true});
	TestFalse(TEXT("Nothing runs after"), Cat->IsOrderRunning());
	Cat->AdvanceMove(1.f);
	TestTrue(TEXT("The walk is dropped"), Cat->GetActorLocation().Equals(FVector::ZeroVector));

	return true;
}

#endif
