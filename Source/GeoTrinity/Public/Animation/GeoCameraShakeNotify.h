// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "GeoCameraShakeNotify.generated.h"

class UCameraShakeBase;

/**
 * Animation notify starting a camera shake on the local player's camera.
 * Authored on the montage rather than fired from C++ so the beat it lands on is placed against the animation, and so
 * every machine playing the montage shakes its own view — an ASC-played montage already replicates, so the shake needs
 * no RPC of its own.
 */
UCLASS(meta = (DisplayName = "Geo Camera Shake"))
class GEOTRINITY_API UGeoCameraShakeNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** Starts ShakeClass on the local player controller; no-op on a dedicated server. */
	virtual void Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation,
						FAnimNotifyEventReference const& EventReference) override;

protected:
	/** Camera shake started when the notify is reached. */
	UPROPERTY(EditAnywhere, Category = "GeoCameraShake")
	TSubclassOf<UCameraShakeBase> ShakeClass;

	/** Multiplier on the amplitudes the shake itself authors. */
	UPROPERTY(EditAnywhere, Category = "GeoCameraShake", meta = (ClampMin = "0.0"))
	float Scale = 1.f;
};
