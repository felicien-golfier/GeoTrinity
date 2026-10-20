// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

/** A begun listen-server world, for a test that spawns actors the way a match does. */
inline UWorld* CreateTestWorld()
{
	UWorld* World = UWorld::CreateWorld(EWorldType::PIE, false);
	World->SetPlayInEditorInitialNetMode(NM_ListenServer);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::PIE);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();
	return World;
}

inline void DestroyTestWorld(UWorld* World)
{
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
}

/** Moves World's timers on by Seconds, as one frame does. A timer set outside a tick starts counting at the next one,
 *  and fires once time is strictly past its expiry: tick 0 after setting it, then step a little over its period. */
inline void TickTestTimers(UWorld& World, float const Seconds)
{
	++GFrameCounter;
	World.GetTimerManager().Tick(Seconds);
}
