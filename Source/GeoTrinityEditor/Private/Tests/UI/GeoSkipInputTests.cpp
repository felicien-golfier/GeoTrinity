// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "InputAction.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoSkipInputTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
// The solo tutorial pauses the world under any menu (AGeoPlayerController::OpenPauseMenu): an action that must answer
// there has to trigger while paused, or the Cat line cannot be skipped with F over the Gems menu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSkipInputPausedTest, "GeoTrinity.MenuGuide.SkipKeyWorksWhilePaused",
								 GeoSkipInputTestFlags)

bool FGeoSkipInputPausedTest::RunTest(FString const& /*Parameters*/)
{
	for (TCHAR const* const Path : {TEXT("/Game/Input/InputActions/IA_InteractUI.IA_InteractUI"),
									TEXT("/Game/Input/InputActions/IA_ToggleMenu.IA_ToggleMenu")})
	{
		UInputAction const* const Action = LoadObject<UInputAction>(nullptr, Path);
		if (TestNotNull(*FString::Printf(TEXT("%s loads"), Path), Action))
		{
			TestTrue(*FString::Printf(TEXT("%s triggers while the game is paused"), Path), Action->bTriggerWhenPaused);
		}
	}

	return true;
}

#endif
