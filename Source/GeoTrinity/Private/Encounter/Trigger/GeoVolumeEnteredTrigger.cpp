// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoVolumeEnteredTrigger.h"

#include "Characters/PlayableCharacter.h"
#include "GameFramework/Actor.h"

void UGeoVolumeEnteredTrigger::OnArm_Implementation()
{
	Entries = 0;
	if (ensureMsgf(Volume, TEXT("%s: no Volume set"), *GetName()))
	{
		Volume->OnActorBeginOverlap.AddDynamic(this, &UGeoVolumeEnteredTrigger::OnVolumeBeginOverlap);
		TArray<AActor*> Inside;
		Volume->GetOverlappingActors(Inside, APlayableCharacter::StaticClass());
		for (AActor* const Player : Inside)
		{
			FireForPlayer(*Player);
		}
	}
}

void UGeoVolumeEnteredTrigger::OnDisarm_Implementation()
{
	if (Volume)
	{
		Volume->OnActorBeginOverlap.RemoveDynamic(this, &UGeoVolumeEnteredTrigger::OnVolumeBeginOverlap);
	}
}

void UGeoVolumeEnteredTrigger::OnVolumeBeginOverlap(AActor* /*OverlappedActor*/, AActor* OtherActor)
{
	if (Cast<APlayableCharacter>(OtherActor))
	{
		FireForPlayer(*OtherActor);
	}
}

void UGeoVolumeEnteredTrigger::FireForPlayer(AActor& Player)
{
	FGeoEncounterPayload Payload;
	Payload.Actor = &Player;
	Payload.Count = ++Entries;
	Fire(Payload);
}
