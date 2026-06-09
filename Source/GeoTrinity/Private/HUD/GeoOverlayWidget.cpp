// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoOverlayWidget.h"

#include "Characters/PlayableCharacter.h"
#include "HUD/GeoAbilityBarWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoOverlayWidget::BuildAbilityBar(AGeoHUD* GeoHUD, APlayableCharacter* PlayableCharacter)
{
	if (!ensureMsgf(AbilityBar, TEXT("UGeoOverlayWidget::BuildAbilityBar — AbilityBar is not bound on %s"), *GetName()))
	{
		return;
	}

	AbilityBar->BuildBar(GeoHUD, PlayableCharacter);
}
