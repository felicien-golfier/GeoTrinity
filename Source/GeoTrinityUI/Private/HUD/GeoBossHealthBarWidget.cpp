// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "HUD/GeoBossHealthBarWidget.h"

#include "Actor/Arena/GeoArena.h"
#include "Components/TextBlock.h"
#include "HUD/HudFunctionLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBossHealthBarWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FightTimerText)
	{
		return;
	}
	if (!Arena.IsValid())
	{
		Arena = AGeoArena::GetFightingArena(this);
	}
	if (AGeoArena const* FightingArena = Arena.Get())
	{
		FightTimerText->SetText(UHudFunctionLibrary::FormatDuration(FightingArena->GetFightElapsedSeconds()));
	}
}
