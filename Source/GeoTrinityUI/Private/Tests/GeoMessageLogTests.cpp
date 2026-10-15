// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMessageSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tool/Visual/GeoColor.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoMessageLogTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** Messages posted at the given times, in order. */
static TArray<FGeoMessage> MakeMessages(TArray<double> const& PostTimes)
{
	TArray<FGeoMessage> Messages;
	for (double const PostTime : PostTimes)
	{
		Messages.Add({INVTEXT("message"), EGeoColor::Neutral, PostTime});
	}

	return Messages;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoMessageLogLayoutTest, "GeoTrinity.MessageLog.Layout", GeoMessageLogTestFlags)

bool FGeoMessageLogLayoutTest::RunTest(FString const& /*Parameters*/)
{
	FGeoMessageLayoutParams const Params{5, 4.f, .5f, .25f};

	TArray<FGeoMessageLine> Lines = UGeoMessageSubsystem::Layout(MakeMessages({0.}), 2., Params);
	TestEqual(TEXT("One message shows"), Lines.Num(), 1);
	TestEqual(TEXT("A lone message sits on the bottom line"), Lines[0].Slot, 0.f);
	TestEqual(TEXT("It is fully visible mid-life"), Lines[0].Alpha, 1.f);

	Lines = UGeoMessageSubsystem::Layout(MakeMessages({0., 1.}), 2., Params);
	TestEqual(TEXT("Two messages show"), Lines.Num(), 2);
	TestEqual(TEXT("The older one keeps the bottom line"), Lines[0].Slot, 0.f);
	TestEqual(TEXT("The newer one appears above"), Lines[1].Slot, 1.f);

	Lines = UGeoMessageSubsystem::Layout(MakeMessages({0., 1.}), 4.125, Params);
	TestEqual(TEXT("The expiring message fades, the other still shows"), Lines.Num(), 2);
	TestTrue(TEXT("The survivor is easing down"), Lines[1].Slot > 0.f && Lines[1].Slot < 1.f);

	Lines = UGeoMessageSubsystem::Layout(MakeMessages({0., 1.}), 4.5, Params);
	TestEqual(TEXT("Only the survivor shows"), Lines.Num(), 1);
	TestEqual(TEXT("The survivor is back on the bottom line"), Lines[0].Slot, 0.f);

	TArray<FGeoMessage> const Six = MakeMessages({0., .1, .2, .3, .4, .5});
	Lines = UGeoMessageSubsystem::Layout(Six, 1., Params);
	TestEqual(TEXT("The sixth message drops the oldest, five show"), Lines.Num(), 5);
	TestEqual(TEXT("The oldest is gone"), Lines[0].MessageIndex, 1);
	TestEqual(TEXT("The newest is on the top line"), Lines.Last().Slot, 4.f);

	Lines = UGeoMessageSubsystem::Layout(MakeMessages({0.}), Params.Lifetime + Params.FadeTime, Params);
	TestEqual(TEXT("Fully faded at Lifetime + FadeTime"), Lines.Num(), 0);

	return true;
}

#endif
