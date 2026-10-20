// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Trigger/GeoGemProfileTrigger.h"

#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemSubsystem.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoGemProfileTrigger::OnArm_Implementation()
{
	Occurrences = 0;
	Profile = UGeoGemSubsystem::GetFirstLocalProfile(GetWorld());
	if (UGeoGemProfileSave* const LocalProfile = Profile.Get())
	{
		if (Event == EGeoGemProfileEvent::Equipped)
		{
			LocalProfile->OnGemEquipped.AddUObject(this, &UGeoGemProfileTrigger::OnGemEquipped);
		}
		else
		{
			LocalProfile->OnGemCrafted.AddUObject(this, &UGeoGemProfileTrigger::OnGemCrafted);
		}
	}
	else
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no local gem profile to watch"), *GetName());
	}
}

void UGeoGemProfileTrigger::OnDisarm_Implementation()
{
	if (UGeoGemProfileSave* const LocalProfile = Profile.Get())
	{
		LocalProfile->OnGemEquipped.RemoveAll(this);
		LocalProfile->OnGemCrafted.RemoveAll(this);
	}
}

void UGeoGemProfileTrigger::OnGemEquipped(EPlayerClass /*PlayerClass*/, int32 /*SocketIndex*/, FName /*GemId*/)
{
	FGeoEncounterPayload Payload;
	Payload.Count = ++Occurrences;
	Fire(Payload);
}

void UGeoGemProfileTrigger::OnGemCrafted(FName /*GemId*/, int32 const Quantity)
{
	FGeoEncounterPayload Payload;
	Payload.Count = ++Occurrences;
	Payload.Value = Quantity;
	Fire(Payload);
}
