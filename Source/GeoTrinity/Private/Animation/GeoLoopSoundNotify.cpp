// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Animation/GeoLoopSoundNotify.h"

#include "Characters/Component/GeoLoopSoundComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UGeoLoopSoundNotify::Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation,
								 FAnimNotifyEventReference const& EventReference)
{
	Super::Notify(MeshComponent, Animation, EventReference);
	UGeoLoopSoundComponent* const LoopSound =
		MeshComponent->GetOwner()->FindComponentByClass<UGeoLoopSoundComponent>();
	if (LoopSound)
	{
		LoopSound->SetPlaying(bPlay, /*bFade*/ false);
	}
}
