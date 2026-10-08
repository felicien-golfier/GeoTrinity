// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoTeamListWidget.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "HUD/GeoPlayerCardWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTeamListWidget::NativeTick(FGeometry const& MyGeometry, float const InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ensureMsgf(RowClass, TEXT("%hs: no RowClass on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	TArray<AGeoPlayerState*> const Teammates = GetTeammates();
	bool bRosterChanged = Teammates.Num() != ShownTeammates.Num();
	for (int32 Index = 0; Index < Teammates.Num() && !bRosterChanged; ++Index)
	{
		bRosterChanged = ShownTeammates[Index].Get() != Teammates[Index];
	}

	if (bRosterChanged)
	{
		RowBox->ClearChildren();
		ShownTeammates.Reset();
		for (AGeoPlayerState* Teammate : Teammates)
		{
			UGeoPlayerCardWidget* Row = CreateWidget<UGeoPlayerCardWidget>(this, RowClass);
			Row->InitForPlayer(Teammate);
			RowBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, RowGap));
			ShownTeammates.Add(Teammate);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AGeoPlayerState*> UGeoTeamListWidget::GetTeammates() const
{
	TArray<AGeoPlayerState*> Teammates;
	AGameStateBase const* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (GameState)
	{
		for (APlayerState* Player : GameState->PlayerArray)
		{
			AGeoPlayerState* GeoPlayer = Cast<AGeoPlayerState>(Player);
			if (GeoPlayer && GeoPlayer != GetOwningPlayerState())
			{
				Teammates.Add(GeoPlayer);
			}
		}
	}
	return Teammates;
}
