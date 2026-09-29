// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"

#include "GeoLoopSoundNotify.generated.h"

/**
 * Animation notify turning the owner's UGeoLoopSoundComponent on or off at once, unfaded, for a montage whose own curves
 * bring the loops in or have already taken them out — a boss intro, which plays before its arena's fight starts the
 * loops, or a boss death, which spins them up and cuts them on its blast. An ASC-played montage already replicates, so
 * every machine runs its own.
 */
UCLASS(meta = (DisplayName = "Geo Loop Sound"))
class GEOTRINITY_API UGeoLoopSoundNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** Starts or stops the owner's loops unfaded. No-op on an owner without the component, like the animation editor's
	 * preview. */
	virtual void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation,
						FAnimNotifyEventReference const& EventReference) override;

	/** Starts the loops; false stops them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoLoopSound")
	bool bPlay = true;
};
