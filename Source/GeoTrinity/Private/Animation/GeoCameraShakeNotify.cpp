// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Animation/GeoCameraShakeNotify.h"

#include "Tool/UGeoGameplayLibrary.h"

void UGeoCameraShakeNotify::Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation,
								   FAnimNotifyEventReference const& EventReference)
{
	Super::Notify(MeshComponent, Animation, EventReference);
	GeoLib::TriggerCameraShake(MeshComponent, ShakeClass, Scale);
}
