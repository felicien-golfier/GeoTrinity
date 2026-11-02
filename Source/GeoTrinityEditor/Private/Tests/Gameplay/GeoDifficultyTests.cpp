// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Data/EffectData.h"
#include "Misc/AutomationTest.h"
#include "Tool/GeoDifficulty.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoDifficultyTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoNightmareEffectMaskTest, "GeoTrinity.Difficulty.NightmareEffectMask",
								 GeoDifficultyTestFlags)

bool FGeoNightmareEffectMaskTest::RunTest(FString const& /*Parameters*/)
{
	FGameplayEffectData Effect;
	TestTrue(TEXT("The default mask applies at Nightmare"), Effect.AppliesAtLevel(GeoDifficultyMask::Nightmare));

	Effect.Difficulties = GeoDifficultyMask::Reduced | GeoDifficultyMask::Original;
	TestFalse(TEXT("A mask without the Nightmare bit does not apply at Nightmare"),
			  Effect.AppliesAtLevel(GeoDifficultyMask::Nightmare));
	TestTrue(TEXT("...and still applies at Original"), Effect.AppliesAtLevel(GeoDifficultyMask::Original));

	Effect.Difficulties |= GeoDifficultyMask::Nightmare;
	TestTrue(TEXT("The Nightmare bit opts it in"), Effect.AppliesAtLevel(GeoDifficultyMask::Nightmare));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
