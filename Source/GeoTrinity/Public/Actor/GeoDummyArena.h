// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoArena.h"
#include "CoreMinimal.h"

#include "GeoDummyArena.generated.h"

/**
 * Arena for a training dummy: it spawns its enemy at level start and then just stands there.
 * AGeoEnemyAIController::TriggerAggro is the only place that starts a match, and it asks IsBoss first — so refusing
 * there keeps the dummy out of the match machinery entirely: no boss health bar, no barrier, no fight commit, no loot.
 * Because it never runs a match, a death at the dummy is always "out of a fight", which the GameState already handles
 * by reviving on the spot — so IsBoss is the only override needed. The dummy still receives its StateTree aggro event
 * and fights back, and like every arena it ends its fight when a match ends anywhere: that is what puts a dummy which
 * chased the players into someone else's fight back on its spot, un-aggroed.
 */
UCLASS()
class GEOTRINITY_API AGeoDummyArena : public AGeoArena
{
	GENERATED_BODY()

public:
	/** Always returns false — the dummy's enemy never triggers a boss fight via TriggerAggro. */
	virtual bool IsBoss(AActor const* Enemy) const override { return false; }
};
