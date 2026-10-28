// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoMontageOwnershipTest, "GeoTrinity.Abilities.MontageOwnership",
								 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeoMontageOwnershipTest::RunTest(FString const& /*Parameters*/)
{
	TestFalse(TEXT("Pattern ability whose pattern has a montage leaves the montage to the pattern"),
			  GetDefault<UGeoTestMontagePatternAbility>()->ShouldPlayAbilityMontage());
	TestTrue(TEXT("Pattern ability whose pattern has no montage keeps the replicated ability montage"),
			 GetDefault<UGeoTestSilentPatternAbility>()->ShouldPlayAbilityMontage());
	TestTrue(TEXT("Plain ability keeps the replicated ability montage"),
			 GetDefault<UGeoGameplayAbility>()->ShouldPlayAbilityMontage());
	return true;
}

#endif
