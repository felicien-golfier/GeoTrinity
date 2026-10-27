// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoTutorialBot.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

/** Which way the Shoot room's quest ends. */
enum class EGeoShootBranch : uint8
{
	/** Twelve kills. */
	Kills,
	/** No kill: the 45 s cut. */
	Cut,
	/** A pause menu held open: nothing respawns and the cut waits. */
	PauseHolds
};

/** How the Special room's rounds go. */
enum class EGeoSpecialBranch : uint8
{
	/** Two rounds without the Special (two stings), the third with it. */
	TwoStings,
	/** The Special in round 1: done, round 2 never spawns. */
	FirstTry,
	/** The Special first, the ally full, the ally dips and is healed again, then the kill: counted once. */
	Order,
	/** Three rounds without the Special: the Cat surrenders and opens the pad. */
	Surrender
};

/** Every room's stages of the main path and of its other branches, mirroring tutorial_encounters.py. The caller adds
 * the start stage (AddStartStage) and the teleports between rooms. A room function begins when the room's encounter
 * runs. */
namespace GeoTutorialStages
{
	/** Movement locked then freed, the Cat shown then hidden after its portal jump. Pad open at the end. */
	void AddLandingStages(FGeoTutorialBot& Bot);
	/** Idle on the open pad: the nag repeats. */
	void AddLandingNagStages(FGeoTutorialBot& Bot);
	/** The pad taken during the last line: the Dash room's first line follows, no Landing line after. */
	void AddEarlyPortalStages(FGeoTutorialBot& Bot);

	/** Falls = 0: the real dash over the void, the stunned line. Falls 1..: that many falls, each a respawn on the
	 * entrance and its line, then the dash and the relief line. */
	void AddDashStages(FGeoTutorialBot& Bot, int32 Falls);
	/** Four idle tiers, each once, then the Cat's Pun jokes. */
	void AddDashIdleStages(FGeoTutorialBot& Bot);

	void AddShootStages(FGeoTutorialBot& Bot, EGeoShootBranch Branch);
	void AddDeployStages(FGeoTutorialBot& Bot);
	void AddSpecialStages(FGeoTutorialBot& Bot, EGeoSpecialBranch Branch);
	/** bDieMidFight: the player dies with the three dummies alive, then the fight goes on; else each dummy shows its
	 * behaviour. */
	void AddDummiesStages(FGeoTutorialBot& Bot, bool bDieMidFight);
	/** The three colours, the lethal wave, then the safe-spot waves: bAllSeries = three runs covering every lane of
	 * every wave (two end in a death and a retry), else the one run that clears. */
	void AddZonesStages(FGeoTutorialBot& Bot, bool bAllSeries);

	/** Boss, summon, zone, ally, wave, points visited, kill, exit pad, loot. */
	void AddMinibossStages(FGeoTutorialBot& Bot);
	/** Three deaths wipe the fight and say their tips in order, a fourth is silent. */
	void AddMinibossWipeStages(FGeoTutorialBot& Bot);
	/** An ally under 30 % health calls out three times, then is silent. */
	void AddMinibossCalloutStages(FGeoTutorialBot& Bot);
	/** The wave kills out of a living dummy's lane and in every lane once no dummy lives. */
	void AddMinibossWaveLaneStages(FGeoTutorialBot& Bot);
	/** The Miniboss killed, the final exit stepped on: the Training Room. */
	void AddFinalExitStages(FGeoTutorialBot& Bot);

	/** The pause menu's Leave tutorial: the Training Room. */
	void AddLeaveFromMenuStages(FGeoTutorialBot& Bot);
	/** The console `Geo.Tutorial.Jump <Argument>` (a room name or number) reloads the map in Room: the jump checks. */
	void AddJumpCommandStages(FGeoTutorialBot& Bot, FString const& Argument, int32 Room);
	/** The console jump to a number past the chain: the tutorial starts at Landing, as a fresh one. */
	void AddJumpPastEndStages(FGeoTutorialBot& Bot);
	/** Leave tutorial, then the tutorial opened again from the Training Room: a fresh one, at Landing, nothing kept. */
	void AddReplayStages(FGeoTutorialBot& Bot);
	/** The jump to Room: the pawn on its start, the earlier abilities granted, Landing never run, the room's step 0. */
	void AddJumpCheckStages(FGeoTutorialBot& Bot, int32 Room);

	/** Four pages popped with the world paused, one per unlock. */
	void AddPageCheckStage(FGeoTutorialBot& Bot);
} // namespace GeoTutorialStages

#endif
