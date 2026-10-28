// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Data/FX/GeoSoundRow.h"
#include "Misc/AutomationTest.h"
#include "Settings/GameDataSettings.h"
#include "Sound/SoundConcurrency.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoSoundConcurrencyTest, "GeoTrinity.Sound.Concurrency",
								 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGeoSoundConcurrencyTest::RunTest(FString const& /*Parameters*/)
{
	USoundConcurrency* const EntryConcurrency = NewObject<USoundConcurrency>(GetTransientPackage());
	USoundConcurrency* const ProjectDefault =
		UGameDataSettings::GetLoadedDataAsset(GetDefault<UGameDataSettings>()->DefaultOneShotConcurrency);

	FGeoSoundEntry Entry;
	TestTrue(TEXT("Project default concurrency is configured"), ProjectDefault != nullptr);
	TestEqual(TEXT("A one-shot without its own concurrency uses the project default"),
			  UGeoSoundRowLibrary::GetConcurrency(Entry, true), ProjectDefault);
	TestNull(TEXT("A loop without its own concurrency has none"), UGeoSoundRowLibrary::GetConcurrency(Entry, false));

	Entry.Concurrency = EntryConcurrency;
	TestEqual(TEXT("A one-shot uses the entry's concurrency"), UGeoSoundRowLibrary::GetConcurrency(Entry, true),
			  EntryConcurrency);
	TestEqual(TEXT("A loop uses the entry's concurrency"), UGeoSoundRowLibrary::GetConcurrency(Entry, false),
			  EntryConcurrency);

	return true;
}

#endif
