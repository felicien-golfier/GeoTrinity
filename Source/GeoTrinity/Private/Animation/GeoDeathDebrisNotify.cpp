// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Animation/GeoDeathDebrisNotify.h"

#include "Characters/PlayableCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"

void UGeoDeathDebrisNotify::Notify(USkeletalMeshComponent* MeshComponent, UAnimSequenceBase* Animation,
								   FAnimNotifyEventReference const& EventReference)
{
	Super::Notify(MeshComponent, Animation, EventReference);
	if (APlayableCharacter* Character = Cast<APlayableCharacter>(MeshComponent->GetOwner()))
	{
		Character->SetDeathDebris(Cast<UNiagaraComponent>(GetSpawnedEffect()));
	}
}
