// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpotMask.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoPatternTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSafeSpotShadowTest, "GeoTrinity.Pattern.SafeSpotShadow", GeoPatternTestFlags)

bool FGeoSafeSpotShadowTest::RunTest(FString const& /*Parameters*/)
{
	FVector2D const Origin(0.f, 0.f);
	FVector2D const Forward(1.f, 0.f);

	FSafeSpot const Pillar{FVector2D(500.f, 0.f), 100.f, ESafeSpotShadow::Behind};
	TestFalse(TEXT("Behind: before the spot is not shielded"), Pillar.Shields(FVector2D(300.f, 0.f), Origin, Forward));
	TestTrue(TEXT("Behind: behind the spot, within its radius, is shielded"),
			 Pillar.Shields(FVector2D(900.f, 50.f), Origin, Forward));
	TestFalse(TEXT("Behind: beside the spot's shadow is not shielded"),
			  Pillar.Shields(FVector2D(900.f, 300.f), Origin, Forward));

	FSafeSpot const Lane{FVector2D(500.f, 200.f), 150.f, ESafeSpotShadow::Lane};
	TestTrue(TEXT("Lane: before the spot, in its width, is shielded"),
			 Lane.Shields(FVector2D(100.f, 250.f), Origin, Forward));
	TestTrue(TEXT("Lane: behind the spot, in its width, is shielded"),
			 Lane.Shields(FVector2D(900.f, 100.f), Origin, Forward));
	TestFalse(TEXT("Lane: outside the spot's width is not shielded"),
			  Lane.Shields(FVector2D(500.f, 400.f), Origin, Forward));

	TestFalse(TEXT("No spots shield nothing"), FSafeSpot::AnyShields({}, FVector2D(900.f, 0.f), Origin, Forward));
	TestTrue(TEXT("Any spot shielding is enough"),
			 FSafeSpot::AnyShields({Lane, Pillar}, FVector2D(900.f, 0.f), Origin, Forward));

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSafeSpotMaskTest, "GeoTrinity.Pattern.SafeSpotMask", GeoPatternTestFlags)

bool FGeoSafeSpotMaskTest::RunTest(FString const& /*Parameters*/)
{
	TArray<FLinearColor> SlotValues = FSafeSpotMask::GetSlotValues({});
	TestEqual(TEXT("Always one value per slot"), SlotValues.Num(), FSafeSpotMask::MaxSlots);
	TestEqual(TEXT("No spot: every slot holds the sentinel"), SlotValues[0].R, -10000.f);
	TestEqual(TEXT("No spot: the last slot too"), SlotValues.Last().R, -10000.f);

	TArray<FSafeSpot> Spots;
	for (int32 Index = 0; Index < FSafeSpotMask::MaxSlots + 2; ++Index)
	{
		Spots.Add({FVector2D(Index * 100.f, 0.f), 50.f, ESafeSpotShadow::Behind});
	}

	AddExpectedMessage(TEXT("safe spots for"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	SlotValues = FSafeSpotMask::GetSlotValues(Spots);
	TestEqual(TEXT("Spots over the slots are skipped"), SlotValues.Num(), FSafeSpotMask::MaxSlots);
	TestEqual(TEXT("A spot lands in its slot"), SlotValues[3].R, 300.f);
	TestEqual(TEXT("The last slot is the eighth spot"), SlotValues.Last().R, 700.f);

	return true;
}

#endif
