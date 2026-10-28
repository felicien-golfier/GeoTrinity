// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTutorialBot.h"
#include "GeoTutorialRoomStages.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

/** The whole tutorial in one session, Landing to Miniboss, the real pads between the rooms, the main branch of each
 * room (the other branches: GeoTutorialPathTests.cpp). The tutorial gate (CLAUDE.md). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTutorialFullRunTest, "GeoTrinity.Tutorial.FullRun", GeoTutorialRunFlags)

bool FGeoTutorialFullRunTest::RunTest(FString const& /*Parameters*/)
{
	TSharedRef<FGeoTutorialBot> Bot = MakeShared<FGeoTutorialBot>(*this);
	Bot->AddStartStage(GeoTutorial::Landing);
	GeoTutorialStages::WatchExitPadsOpenWithLastLine(*Bot);
	GeoTutorialStages::AddLandingStages(*Bot);
	Bot->AddTeleportStages(GeoTutorial::Landing);
	GeoTutorialStages::AddDashStages(*Bot, 0);
	Bot->AddTeleportStages(GeoTutorial::Dash);
	GeoTutorialStages::AddShootStages(*Bot, EGeoShootBranch::Kills);
	Bot->AddTeleportStages(GeoTutorial::Shoot);
	GeoTutorialStages::AddDeployStages(*Bot);
	Bot->AddTeleportStages(GeoTutorial::Deploy);
	GeoTutorialStages::AddSpecialStages(*Bot, EGeoSpecialBranch::TwoStings);
	Bot->AddTeleportStages(GeoTutorial::Special);
	GeoTutorialStages::AddDummiesStages(*Bot, false);
	Bot->AddTeleportStages(GeoTutorial::Dummies);
	GeoTutorialStages::AddZonesStages(*Bot, false);
	Bot->AddTeleportStages(GeoTutorial::Zones);
	GeoTutorialStages::AddMinibossStages(*Bot);
	GeoTutorialStages::AddPageCheckStage(*Bot);
	FGeoTutorialBot::QueueSession(*this, Bot);

	return true;
}

#endif
