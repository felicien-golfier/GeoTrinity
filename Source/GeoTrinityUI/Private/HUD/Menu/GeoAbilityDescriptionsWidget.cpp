// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoAbilityDescriptionsWidget.h"

#include "Characters/PlayableCharacter.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "HUD/Menu/GeoAbilityCardWidget.h"
#include "HUD/Menu/GeoAbilityDetailWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Style/GeoShape.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoAbilityDescriptionsWidget::HandleBack);

	BuildCards();
	CloseDetail();
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoAbilityDescriptionsWidget::GetInitialFocusWidget() const
{
	return BackButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoAbilityDescriptionsWidget::HandleBackAction()
{
	if (SelectedIndex != INDEX_NONE)
	{
		CloseDetail();
	}
	else
	{
		HandleBack();
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoAbilityDescriptionsWidget::NativeOnMouseButtonDown(FGeometry const& InGeometry,
															  FPointerEvent const& InMouseEvent)
{
	if (SelectedIndex != INDEX_NONE)
	{
		CloseDetail();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::BuildCards()
{
	APlayableCharacter* PlayableCharacter = Cast<APlayableCharacter>(GetOwningPlayerPawn());
	if (!ensureMsgf(CardClass && CardGrid, TEXT("%hs: no CardClass or CardGrid on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}
	if (!PlayableCharacter)
	{
		UE_LOG(LogTemp, Log, TEXT("UGeoAbilityDescriptionsWidget: no playable pawn yet, page left empty"));
		return;
	}

	CardGrid->ClearChildren();
	ShowClass(PlayableCharacter->GetPlayerClass());
	Cards = TArray<TObjectPtr<UGeoAbilityCardWidget>>(
		UGeoAbilityCardWidget::CreateClassCards(*this, CardClass, *PlayableCharacter));
	for (int32 CardIndex = 0; CardIndex < Cards.Num(); ++CardIndex)
	{
		UUniformGridSlot* CardSlot =
			CardGrid->AddChildToUniformGrid(Cards[CardIndex], CardIndex / CardColumns, CardIndex % CardColumns);
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
		Cards[CardIndex]->SetIsFocusable(true);
		Cards[CardIndex]->OnSelected.AddUniqueDynamic(this, &UGeoAbilityDescriptionsWidget::HandleCardSelected);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::HandleCardSelected(UGeoAbilityCardWidget* Card)
{
	if (!ensureMsgf(DetailWidget, TEXT("%hs: no DetailWidget on %s"), __FUNCTION__, *GetName()))
	{
		return;
	}

	SelectedIndex = Cards.IndexOfByKey(Card);
	Card->SetSelected(true);
	DetailWidget->CopyAbility(*Card);
	DetailWidget->Open();
	if (DetailScrim)
	{
		DetailScrim->SetVisibility(ESlateVisibility::Visible);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::CloseDetail()
{
	SelectedIndex = INDEX_NONE;
	for (UGeoAbilityCardWidget* Card : Cards)
	{
		Card->SetSelected(false);
	}

	if (DetailWidget)
	{
		DetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (DetailScrim)
	{
		DetailScrim->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityDescriptionsWidget::ShowClass(EPlayerClass const PlayerClass)
{
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(PlayerClass) : nullptr;
	if (!ClassStyle)
	{
		return;
	}

	if (ClassShape)
	{
		ClassShape->SetClassShape(*ClassStyle);
	}
	if (ClassNameText)
	{
		ClassNameText->SetText(ClassStyle->Name);
	}
	if (ClassRoleText)
	{
		ClassRoleText->SetText(ClassStyle->Role);
	}
}
