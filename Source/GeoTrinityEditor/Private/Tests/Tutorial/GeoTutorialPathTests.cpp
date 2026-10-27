// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTutorialBot.h"
#include "GeoTutorialRoomStages.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace GeoTutorialPaths
{
	/** One branch of a room: the room the bot is jumped into (INDEX_NONE: none) and the stages that play it. */
	struct FPathCase
	{
		FString Name;
		int32 StartRoom;
		TFunction<void(FGeoTutorialBot&)> Build;
	};

	TArray<FPathCase> GetPathCases()
	{
		using namespace GeoTutorialStages;
		return {
			{TEXT("LandingNag"), GeoTutorial::Landing, &AddLandingNagStages},
			{TEXT("EarlyPortal"), GeoTutorial::Landing, &AddEarlyPortalStages},
			{TEXT("DashFalls"), GeoTutorial::Dash,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddDashStages(Bot, 7);
			 }},
			{TEXT("DashIdle"), GeoTutorial::Dash, &AddDashIdleStages},
			{TEXT("ShootCut"), GeoTutorial::Shoot,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddShootStages(Bot, EGeoShootBranch::Cut);
			 }},
			{TEXT("PauseHolds"), GeoTutorial::Shoot,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddShootStages(Bot, EGeoShootBranch::PauseHolds);
			 }},
			{TEXT("SpecialFirstTry"), GeoTutorial::Special,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddSpecialStages(Bot, EGeoSpecialBranch::FirstTry);
			 }},
			{TEXT("SpecialOrder"), GeoTutorial::Special,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddSpecialStages(Bot, EGeoSpecialBranch::Order);
			 }},
			{TEXT("SpecialSurrender"), GeoTutorial::Special,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddSpecialStages(Bot, EGeoSpecialBranch::Surrender);
			 }},
			{TEXT("DummiesDeath"), GeoTutorial::Dummies,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddDummiesStages(Bot, true);
			 }},
			{TEXT("SafeSpotsRetry"), GeoTutorial::Zones,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddZonesStages(Bot, true);
			 }},
			{TEXT("MinibossWipe"), GeoTutorial::Miniboss, &AddMinibossWipeStages},
			{TEXT("MinibossCallouts"), GeoTutorial::Miniboss, &AddMinibossCalloutStages},
			{TEXT("MinibossWaveLanes"), GeoTutorial::Miniboss, &AddMinibossWaveLaneStages},
			{TEXT("FinalExit"), GeoTutorial::Miniboss, &AddFinalExitStages},
			{TEXT("LeaveFromMenu"), GeoTutorial::Dash, &AddLeaveFromMenuStages},
			{TEXT("JumpByNumber"), INDEX_NONE,
			 [](FGeoTutorialBot& Bot)
			 {
				 AddJumpCommandStages(Bot, FString::FromInt(GeoTutorial::Zones), GeoTutorial::Zones);
			 }},
			{TEXT("JumpPastEnd"), INDEX_NONE, &AddJumpPastEndStages},
			{TEXT("Replay"), GeoTutorial::Dash, &AddReplayStages},
		};
	}
} // namespace GeoTutorialPaths

/** Every other branch of the tutorial, one session each, entered by the jump of the room it belongs to. */
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGeoTutorialPathTest, "GeoTrinity.Tutorial.Path", GeoTutorialRunFlags)

void FGeoTutorialPathTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (GeoTutorialPaths::FPathCase const& Case : GeoTutorialPaths::GetPathCases())
	{
		OutBeautifiedNames.Add(Case.Name);
		OutTestCommands.Add(Case.Name);
	}
}

bool FGeoTutorialPathTest::RunTest(FString const& Parameters)
{
	TArray<GeoTutorialPaths::FPathCase> const Cases = GeoTutorialPaths::GetPathCases();
	GeoTutorialPaths::FPathCase const* const Case = Cases.FindByPredicate(
		[&Parameters](GeoTutorialPaths::FPathCase const& Candidate)
		{
			return Candidate.Name == Parameters;
		});
	if (TestNotNull(*FString::Printf(TEXT("Path case '%s'"), *Parameters), Case))
	{
		TSharedRef<FGeoTutorialBot> Bot = MakeShared<FGeoTutorialBot>(*this);
		Bot->AddStartStage(Case->StartRoom);
		Case->Build(*Bot);
		FGeoTutorialBot::QueueSession(*this, Bot);
	}

	return true;
}

/** The console jump to each room by its name: the real travel, then the jump checks. */
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGeoTutorialJumpTest, "GeoTrinity.Tutorial.Jump", GeoTutorialRunFlags)

void FGeoTutorialJumpTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	for (int32 Room = GeoTutorial::Dash; Room < GeoTutorial::RoomCount; ++Room)
	{
		OutBeautifiedNames.Add(GeoTutorial::RoomName(Room));
		OutTestCommands.Add(FString::FromInt(Room));
	}
}

bool FGeoTutorialJumpTest::RunTest(FString const& Parameters)
{
	int32 const Room = FCString::Atoi(*Parameters);
	TSharedRef<FGeoTutorialBot> Bot = MakeShared<FGeoTutorialBot>(*this);
	Bot->AddStartStage(INDEX_NONE);
	GeoTutorialStages::AddJumpCommandStages(*Bot, GeoTutorial::RoomName(Room), Room);
	FGeoTutorialBot::QueueSession(*this, Bot);

	return true;
}

#endif
