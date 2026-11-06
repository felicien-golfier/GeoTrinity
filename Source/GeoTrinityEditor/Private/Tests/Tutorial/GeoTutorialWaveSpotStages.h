// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoTutorialBot.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace GeoTutorialStages
{
	/**
	 * The Zones room's lateral wave must kill whoever stands in it: about 20 spots spread over the room, one stage and
	 * one death each. WaveNumber 1 is the lethal wave (the LethalWave step, no safe spot); 2 to 4 are the three
	 * safe-spot waves: the pawn survives the waves before in their first lane, then stands at the spot, outside every
	 * lane of the wave. A wave that ends with the pawn alive fails the test, naming the wave and the spot, and the run
	 * goes on to the next spot. The encounter is sent to the step by GoToStep, so no run waits for the earlier steps.
	 *
	 * bAfterColors (WaveNumber 1 only): the step is reached the way a player reaches it, the whole colour timeline
	 * played (the pawn standing in the Shield and Heal overlap), and the pawn moves to the spot as the lethal wave step
	 * begins.
	 */
	void AddWaveSpotStages(FGeoTutorialBot& Bot, int32 WaveNumber, bool bAfterColors);
} // namespace GeoTutorialStages

#endif
