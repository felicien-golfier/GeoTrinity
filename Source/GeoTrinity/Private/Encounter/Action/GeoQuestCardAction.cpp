// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoQuestCardAction.h"

#include "HUD/Interface/GeoHUDInterface.h"

void UGeoQuestCardAction::Execute_Implementation(FGeoEncounterPayload const& Payload)
{
	if (IGeoHUDInterface* const HUD = GetLocalHUD())
	{
		switch (Operation)
		{
		case EGeoQuestCardOperation::Show:
			HUD->ShowQuestCard(Title, Max, TimeLimitSeconds);
			bCardShown = true;
			break;
		case EGeoQuestCardOperation::Count:
			HUD->SetQuestCount(bShowMax ? Max : Payload.Count);
			break;
		case EGeoQuestCardOperation::Hide:
			HUD->HideQuestCard();
			break;
		}
	}
}

void UGeoQuestCardAction::OnCancel_Implementation()
{
	if (bCardShown)
	{
		bCardShown = false;
		if (IGeoHUDInterface* const HUD = GetLocalHUD())
		{
			HUD->HideQuestCard();
		}
	}
}
