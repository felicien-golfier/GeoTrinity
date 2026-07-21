// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoArena.h"
#include "Characters/PlayableCharacter.h"
#include "CoreMinimal.h"

#include "GeoDummyArena.generated.h"

/**
 * Arena for a training dummy: it spawns its enemy at level start and then just stands there.
 * AGeoEnemyAIController::TriggerAggro is the only place that starts a match, and it asks IsBoss first — so refusing
 * there keeps the dummy out of the match machinery entirely: no boss health bar, no barrier, no fight commit, no loot.
 * The dummy still receives its StateTree aggro event and fights back. Note the hub is one of these and is still the
 * GameState's ActiveArena at level start — being the active arena is not the same as running a match.
 */
UCLASS()
class GEOTRINITY_API AGeoDummyArena : public AGeoArena
{
	GENERATED_BODY()

public:
	/** Always returns false — the dummy's enemy never triggers a boss fight via TriggerAggro. */
	virtual bool IsBoss(AActor const* Enemy) const override { return false; }

	/** There is no wipe to wait for at a dummy: whoever dies gets straight back up where they fell. */
	virtual void RespawnPlayer(APlayableCharacter& Player) override { Player.Revive(); }
};
