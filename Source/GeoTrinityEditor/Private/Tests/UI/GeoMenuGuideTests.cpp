// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMenuGuide.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoMenuGuideTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoMenuGuideScrimTest, "GeoTrinity.MenuGuide.Scrim", GeoMenuGuideTestFlags)

bool FGeoMenuGuideScrimTest::RunTest(FString const& /*Parameters*/)
{
	FSlateRect const Canvas(0.f, 0.f, 100.f, 80.f);
	TArray<FSlateRect> Scrim = GeoMenuGuide::MakeScrim(Canvas, FSlateRect(20.f, 10.f, 60.f, 50.f));
	TestEqual(TEXT("A hole inside leaves four parts"), Scrim.Num(), 4);
	float Area = 0.f;
	for (FSlateRect const& Part : Scrim)
	{
		Area += Part.GetArea();
		TestFalse(TEXT("No part covers the hole's centre"), Part.ContainsPoint(FVector2f(40.f, 30.f)));
	}

	TestEqual(TEXT("Parts and hole cover the canvas exactly"), Area + 40.f * 40.f, 100.f * 80.f);

	Scrim = GeoMenuGuide::MakeScrim(Canvas, FSlateRect(0.f, 0.f, 100.f, 30.f));
	TestEqual(TEXT("A hole spanning the top leaves only the part under it"), Scrim.Num(), 1);

	Scrim = GeoMenuGuide::MakeScrim(Canvas, FSlateRect(-20.f, -20.f, 40.f, 40.f));
	float ClampedArea = 0.f;
	for (FSlateRect const& Part : Scrim)
	{
		ClampedArea += Part.GetArea();
	}

	TestEqual(TEXT("A hole past the canvas edge is clamped to it"), ClampedArea + 40.f * 40.f, 100.f * 80.f);

	FGeoMenuGuideHintPlace Place = GeoMenuGuide::PlaceHint(Canvas, FSlateRect(10.f, 40.f, 30.f, 60.f), 20.f, 5.f);
	TestEqual(TEXT("Room above: the label hangs over the hole"), Place.Position.Y, 35.f);
	TestEqual(TEXT("Its bottom edge is the anchor"), Place.Alignment.Y, 1.f);
	TestEqual(TEXT("Left half: it starts on the hole's left edge"), Place.Position.X, 10.f);

	Place = GeoMenuGuide::PlaceHint(Canvas, FSlateRect(70.f, 5.f, 90.f, 20.f), 20.f, 5.f);
	TestEqual(TEXT("No room above: the label goes under the hole"), Place.Position.Y, 25.f);
	TestEqual(TEXT("Its top edge is the anchor"), Place.Alignment.Y, 0.f);
	TestEqual(TEXT("Right half: it ends on the hole's right edge"), Place.Position.X, 90.f);
	TestEqual(TEXT("and grows leftwards"), Place.Alignment.X, 1.f);
	return true;
}

#endif
