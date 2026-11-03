// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "System/Stats/GeoLeaderboardSave.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoLeaderboardTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

static FGeoAttemptEntry MakeAttempt(float const DurationSeconds, TArray<FString> const& PlayerNames,
									EGeoDifficulty const Difficulty = EGeoDifficulty::Original,
									FGameplayTag const ArenaTag = FGameplayTag())
{
	FGeoAttemptEntry Entry;
	Entry.AttemptId = FGuid::NewGuid();
	Entry.ArenaTag = ArenaTag;
	Entry.Difficulty = Difficulty;
	Entry.DurationSeconds = DurationSeconds;
	Entry.BossHealthRatio = 0.f;
	for (FString const& Name : PlayerNames)
	{
		Entry.Players.AddDefaulted_GetRef().PlayerName = Name;
	}
	return Entry;
}

static bool IsListed(UGeoLeaderboardSave const* Leaderboard, FGeoAttemptEntry const& Entry)
{
	return Leaderboard->Entries.ContainsByPredicate(
		[&Entry](FGeoAttemptEntry const& Recorded)
		{
			return Recorded.AttemptId == Entry.AttemptId;
		});
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLeaderboardKeepsBestTest, "GeoTrinity.Leaderboard.KeepsBestPerPlayer",
								 GeoLeaderboardTestFlags)
bool FGeoLeaderboardKeepsBestTest::RunTest(FString const& /*Parameters*/)
{
	UGeoLeaderboardSave* Leaderboard = NewObject<UGeoLeaderboardSave>();
	FGeoAttemptEntry const First = MakeAttempt(100.f, {"A", "B"});
	TestTrue("first attempt kept", Leaderboard->AddAttempt(First));

	FGeoAttemptEntry const Slower = MakeAttempt(120.f, {"A", "B"});
	TestFalse("slower same group dropped", Leaderboard->AddAttempt(Slower));
	TestEqual("one row", Leaderboard->Entries.Num(), 1);

	FGeoAttemptEntry const Faster = MakeAttempt(80.f, {"A", "B"});
	TestTrue("faster same group kept", Leaderboard->AddAttempt(Faster));
	TestEqual("replaces the old row", Leaderboard->Entries.Num(), 1);
	TestFalse("old row gone", IsListed(Leaderboard, First));

	FGeoAttemptEntry const Tie = MakeAttempt(80.f, {"A", "B"});
	TestFalse("tie keeps the earlier", Leaderboard->AddAttempt(Tie));
	TestTrue("earlier still listed", IsListed(Leaderboard, Faster));

	TestTrue("same AttemptId again stays one", Leaderboard->AddAttempt(Faster));
	TestEqual("still one row", Leaderboard->Entries.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLeaderboardGroupsTest, "GeoTrinity.Leaderboard.GroupsAndSolos",
								 GeoLeaderboardTestFlags)
bool FGeoLeaderboardGroupsTest::RunTest(FString const& /*Parameters*/)
{
	UGeoLeaderboardSave* Leaderboard = NewObject<UGeoLeaderboardSave>();
	FGeoAttemptEntry const WithC = MakeAttempt(100.f, {"A", "B", "C"});
	Leaderboard->AddAttempt(WithC);
	FGeoAttemptEntry const WithD = MakeAttempt(80.f, {"A", "B", "D"});
	Leaderboard->AddAttempt(WithD);
	TestEqual("two rows", Leaderboard->Entries.Num(), 2);
	TestTrue("C keeps the old row", IsListed(Leaderboard, WithC));
	TestTrue("A, B and D share the new one", IsListed(Leaderboard, WithD));

	FGeoAttemptEntry const Solo = MakeAttempt(50.f, {"A"});
	Leaderboard->AddAttempt(Solo);
	TestTrue("solo run listed", IsListed(Leaderboard, Solo));
	TestTrue("B and D keep the group row", IsListed(Leaderboard, WithD));
	TestTrue("C keeps the old row", IsListed(Leaderboard, WithC));
	TestEqual("three rows", Leaderboard->Entries.Num(), 3);
	TestEqual("best first", Leaderboard->Entries[0].AttemptId, Solo.AttemptId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLeaderboardSeparateTest, "GeoTrinity.Leaderboard.PerBossAndDifficulty",
								 GeoLeaderboardTestFlags)
bool FGeoLeaderboardSeparateTest::RunTest(FString const& /*Parameters*/)
{
	UGeoLeaderboardSave* Leaderboard = NewObject<UGeoLeaderboardSave>();
	FGameplayTag const OtherArena = FGameplayTag::RequestGameplayTag("Arena", false);
	FGeoAttemptEntry const Original = MakeAttempt(50.f, {"A"});
	FGeoAttemptEntry const Safe = MakeAttempt(200.f, {"A"}, EGeoDifficulty::Safe);
	FGeoAttemptEntry const Elsewhere = MakeAttempt(300.f, {"A"}, EGeoDifficulty::Original, OtherArena);
	Leaderboard->AddAttempt(Original);
	FGeoAttemptEntry const Nightmare = MakeAttempt(250.f, {"A"}, EGeoDifficulty::Nightmare);
	Leaderboard->AddAttempt(Safe);
	Leaderboard->AddAttempt(Nightmare);
	Leaderboard->AddAttempt(Elsewhere);
	TestTrue("original kept", IsListed(Leaderboard, Original));
	TestTrue("nightmare never compared with original", IsListed(Leaderboard, Nightmare));
	TestTrue("other difficulty never compared", IsListed(Leaderboard, Safe));
	if (OtherArena.IsValid())
	{
		TestTrue("other boss never compared", IsListed(Leaderboard, Elsewhere));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLeaderboardLegacyTest, "GeoTrinity.Leaderboard.LegacyListCollapses",
								 GeoLeaderboardTestFlags)
bool FGeoLeaderboardLegacyTest::RunTest(FString const& /*Parameters*/)
{
	UGeoLeaderboardSave* Leaderboard = NewObject<UGeoLeaderboardSave>();
	FGeoAttemptEntry const Slow = MakeAttempt(120.f, {"A", "B"});
	FGeoAttemptEntry const Fast = MakeAttempt(80.f, {"A", "B"});
	FGeoAttemptEntry const Other = MakeAttempt(90.f, {"C"});
	Leaderboard->Entries = {Slow, Other, Fast};
	Leaderboard->Prune();
	TestEqual("one row per best", Leaderboard->Entries.Num(), 2);
	TestTrue("fast kept", IsListed(Leaderboard, Fast));
	TestFalse("slow dropped", IsListed(Leaderboard, Slow));
	TestTrue("other player kept", IsListed(Leaderboard, Other));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLeaderboardEchoLevelTest, "GeoTrinity.Leaderboard.EchoLevelSurvivesSave",
								 GeoLeaderboardTestFlags)
bool FGeoLeaderboardEchoLevelTest::RunTest(FString const& /*Parameters*/)
{
	UGeoLeaderboardSave* Leaderboard = NewObject<UGeoLeaderboardSave>();
	FGeoAttemptEntry Echoed = MakeAttempt(100.f, {"A", "B"});
	Echoed.Players[0].Level = 20;
	Echoed.Players[0].EchoLevel = 7;
	FGeoAttemptEntry const Faster = MakeAttempt(80.f, {"C"});
	Leaderboard->AddAttempt(Echoed);
	Leaderboard->AddAttempt(Faster);
	TestEqual("pruning and sorting keep the echo level", Leaderboard->Entries[1].Players[0].EchoLevel, 7);
	TestEqual("a player without one reads 0", Leaderboard->Entries[1].Players[1].EchoLevel, 0);

	TArray<uint8> Bytes;
	if (TestTrue("saved", UGameplayStatics::SaveGameToMemory(Leaderboard, Bytes)))
	{
		UGeoLeaderboardSave const* const Loaded =
			Cast<UGeoLeaderboardSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
		if (TestNotNull("loaded", Loaded) && TestEqual("both rows back", Loaded->Entries.Num(), 2))
		{
			TestEqual("echo level round trips", Loaded->Entries[1].Players[0].EchoLevel, 7);
			TestEqual("class level round trips", Loaded->Entries[1].Players[0].Level, 20);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoLeaderboardWipeTest, "GeoTrinity.Leaderboard.BuildWipe", GeoLeaderboardTestFlags)
bool FGeoLeaderboardWipeTest::RunTest(FString const& /*Parameters*/)
{
	UGeoLeaderboardSave* Leaderboard = NewObject<UGeoLeaderboardSave>();
	Leaderboard->AddAttempt(MakeAttempt(100.f, {"A"}));

	TestFalse("never wipes when the setting is off", Leaderboard->ApplyBuildWipe(false, "Build1"));
	TestEqual("kept", Leaderboard->Entries.Num(), 1);

	TestTrue("wipes on a new stamp", Leaderboard->ApplyBuildWipe(true, "Build1"));
	TestEqual("emptied", Leaderboard->Entries.Num(), 0);

	Leaderboard->AddAttempt(MakeAttempt(100.f, {"A"}));
	TestFalse("same build keeps its runs", Leaderboard->ApplyBuildWipe(true, "Build1"));
	TestEqual("runs kept", Leaderboard->Entries.Num(), 1);

	TestTrue("next build wipes again", Leaderboard->ApplyBuildWipe(true, "Build2"));
	return true;
}

#endif
