// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AnimNotify_PlayNiagaraEffect.h"
#include "CoreMinimal.h"

#include "GeoDeathDebrisNotify.generated.h"

/**
 * Death montage notify spawning the debris a character sheds as it goes. Authored on the montage like any Niagara
 * notify, so the burst lands on the beat of the animation and previews in the montage editor; it then hands the
 * spawned system to its APlayableCharacter, which keeps it until the revive clears it — the debris system itself must never
 * end on its own.
 */
UCLASS(meta = (DisplayName = "Geo Death Debris"))
class GEOTRINITY_API UGeoDeathDebrisNotify : public UAnimNotify_PlayNiagaraEffect
{
	GENERATED_BODY()

public:
	/** Spawns the debris, then gives it to the owning character when there is one (not in an editor preview). */
	virtual void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation,
						FAnimNotifyEventReference const& EventReference) override;
};
