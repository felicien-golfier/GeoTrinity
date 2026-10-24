// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Tutorial/GeoCatFaceMotion.h"
#include "HUD/Tutorial/GeoCatLineMarkup.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoCatLineTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatLineMarkupTest, "GeoTrinityUI.Tutorial.CatLineMarkup", GeoCatLineTestFlags)

bool FGeoCatLineMarkupTest::RunTest(FString const& /*Parameters*/)
{
	FGameplayTag const DashTag = FGeoGameplayTags::Get().InputTag_Dash;
	FGameplayTag AskedTag;
	auto const KeyLabel = [&AskedTag](FGameplayTag const InputTag)
	{
		AskedTag = InputTag;
		return INVTEXT("SPACE");
	};

	TestEqual(TEXT("Bold"), GeoCatLine::ToRichText(TEXT("press **now** please"), KeyLabel),
			  FString(TEXT("press <Bold>now</> please")));
	TestEqual(TEXT("Shake"), GeoCatLine::ToRichText(TEXT("you ~~died~~ again"), KeyLabel),
			  FString(TEXT("you <Shake>died</> again")));
	TestEqual(TEXT("Key token"),
			  GeoCatLine::ToRichText(FString::Printf(TEXT("press **{Key:%s}**"), *DashTag.ToString()), KeyLabel),
			  FString(TEXT("press <Bold>SPACE</>")));
	TestEqual(TEXT("The key token asks for its input tag"), AskedTag, DashTag);
	TestEqual(TEXT("Unknown token"), GeoCatLine::ToRichText(TEXT("a {Foo} b"), KeyLabel), FString(TEXT("a {Foo} b")));
	TestEqual(TEXT("Key token naming no tag"), GeoCatLine::ToRichText(TEXT("{Key:Not.A.Real.Tag}"), KeyLabel),
			  FString(TEXT("{Key:Not.A.Real.Tag}")));
	TestEqual(TEXT("Unclosed brace"), GeoCatLine::ToRichText(TEXT("a {b"), KeyLabel), FString(TEXT("a {b")));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatLineRevealTest, "GeoTrinityUI.Tutorial.CatLineReveal", GeoCatLineTestFlags)

bool FGeoCatLineRevealTest::RunTest(FString const& /*Parameters*/)
{
	FString const Bold = TEXT("press <Bold>now</> please");
	TestEqual(TEXT("Plain text"), GeoCatLine::RevealRichText(TEXT("hello"), 3), FString(TEXT("hel")));
	TestEqual(TEXT("Nothing visible"), GeoCatLine::RevealRichText(Bold, 0), FString());
	TestEqual(TEXT("Run cut mid-way is closed"), GeoCatLine::RevealRichText(Bold, 8),
			  FString(TEXT("press <Bold>no</>")));
	TestEqual(TEXT("Run ended exactly"), GeoCatLine::RevealRichText(Bold, 9), FString(TEXT("press <Bold>now</>")));
	TestEqual(TEXT("Past the run"), GeoCatLine::RevealRichText(Bold, 11), FString(TEXT("press <Bold>now</> p")));
	TestEqual(TEXT("Key label counts as its letters"), GeoCatLine::RevealRichText(TEXT("<Bold>SPACE</>"), 2),
			  FString(TEXT("<Bold>SP</>")));
	TestEqual(TEXT("Count past the end"), GeoCatLine::RevealRichText(Bold, 1000), Bold);
	TestEqual(TEXT("Visible text"), GeoCatLine::VisibleText(Bold), FString(TEXT("press now please")));

	FString const Visible = TEXT("  one  two\nthree ");
	TArray<FStringView> const Words = GeoCatLine::FindWords(Visible);
	TestEqual(TEXT("Word count"), Words.Num(), 3);
	TestTrue(TEXT("Words split on whitespace"),
			 Words.Num() == 3 && Words[0].Equals(TEXT("one")) && Words[1].Equals(TEXT("two"))
				 && Words[2].Equals(TEXT("three")));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatLinePitchTest, "GeoTrinityUI.Tutorial.CatLinePitch", GeoCatLineTestFlags)

bool FGeoCatLinePitchTest::RunTest(FString const& /*Parameters*/)
{
	constexpr float Min = 0.8f;
	constexpr float Max = 1.4f;
	TestEqual(TEXT("Same word, same pitch"), GeoCatLine::PitchForWord(TEXT("dash"), Min, Max),
			  GeoCatLine::PitchForWord(TEXT("dash"), Min, Max));
	TestEqual(TEXT("Case does not matter"), GeoCatLine::PitchForWord(TEXT("Dash"), Min, Max),
			  GeoCatLine::PitchForWord(TEXT("dASH"), Min, Max));

	TSet<float> Pitches;
	bool bAllInRange = true;
	for (TCHAR Letter = TCHAR('a'); Letter <= TCHAR('z'); ++Letter)
	{
		FString const Word = FString::Printf(TEXT("word%c%c"), Letter, Letter);
		float const Pitch = GeoCatLine::PitchForWord(Word, Min, Max);
		bAllInRange &= Pitch >= Min && Pitch <= Max;
		Pitches.Add(Pitch);
	}

	TestTrue(TEXT("Pitches stay in range"), bAllInRange);
	TestTrue(TEXT("26 words give at least 10 distinct pitches"), Pitches.Num() >= 10);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatFaceMotionTest, "GeoTrinityUI.Tutorial.CatFaceMotion", GeoCatLineTestFlags)

bool FGeoCatFaceMotionTest::RunTest(FString const& /*Parameters*/)
{
	TestEqual(TEXT("Eyes open at the start"), GeoCatFaceMotion::GetEyeScale(0.), 1.f);
	TestEqual(TEXT("Eyes still open at 40 %"), GeoCatFaceMotion::GetEyeScale(.4 * 5.4), 1.f, .001f);
	TestEqual(TEXT("Ear still at rest at 86 %"), GeoCatFaceMotion::GetEarAngle(.86 * 5.5, 0), 0.f, .001f);
	TestEqual(TEXT("Eyes squint at half the cycle"), GeoCatFaceMotion::GetEyeScale(.5 * 5.4), .55f);
	TestEqual(TEXT("Eyes blink at 94 %"), GeoCatFaceMotion::GetEyeScale(.94 * 5.4), .08f, .001f);
	TestEqual(TEXT("Eyes loop"), GeoCatFaceMotion::GetEyeScale(5.4 + .5 * 5.4), .55f);
	TestEqual(TEXT("Ear at rest at the start"), GeoCatFaceMotion::GetEarAngle(0., 0), 0.f);
	TestEqual(TEXT("Ear flicks back at 90 %"), GeoCatFaceMotion::GetEarAngle(.9 * 5.5, 0), -5.f, .001f);
	TestEqual(TEXT("Second ear follows .3 s later"), GeoCatFaceMotion::GetEarAngle(.9 * 5.5 + .3, 1), -5.f, .001f);
	TestEqual(TEXT("Mouth closed at the start"), GeoCatFaceMotion::GetMouthScale(0.), .35f);
	TestEqual(TEXT("Mouth open after .38 s"), GeoCatFaceMotion::GetMouthScale(.38), 1.f, .001f);
	TestEqual(TEXT("Mouth closes again after .76 s"), GeoCatFaceMotion::GetMouthScale(.76), .35f, .001f);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDefaultMappedKeyTest, "GeoTrinityUI.Tutorial.DefaultMappedKey",
								 GeoCatLineTestFlags)

bool FGeoDefaultMappedKeyTest::RunTest(FString const& /*Parameters*/)
{
	UInputAction* const Mapped = NewObject<UInputAction>();
	UInputAction* const Unmapped = NewObject<UInputAction>();
	UInputMappingContext* const Context = NewObject<UInputMappingContext>();
	Context->MapKey(Mapped, EKeys::F);

	TestEqual(TEXT("Mapped action"), UHudFunctionLibrary::GetDefaultMappedKey(Context, Mapped), FKey(EKeys::F));
	TestFalse(TEXT("Unmapped action"), UHudFunctionLibrary::GetDefaultMappedKey(Context, Unmapped).IsValid());
	TestFalse(TEXT("No context"), UHudFunctionLibrary::GetDefaultMappedKey(nullptr, Mapped).IsValid());
	return true;
}

#endif
