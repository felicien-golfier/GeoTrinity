// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoGemsWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/LocalPlayer.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemSubsystem.h"
#include "HUD/Menu/GeoGemForgeWidget.h"
#include "HUD/Menu/GeoGemLoadoutWidget.h"
#include "HUD/Menu/GeoListRowWidget.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	LoadoutPage->SetClassTabBox(ClassTabBox);
	LoadoutPage->OnProfileChanged.AddWeakLambda(this,
												[this]
												{
													ShowShards();
												});
	ForgePage->OnProfileChanged.AddWeakLambda(this,
											  [this]
											  {
												  ShowShards();
											  });
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ShowHeaderTabs();
	ShowShards();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemsWidget::Refresh()
{
	LoadoutPage->Refresh();
	ForgePage->Refresh();
	ShowShards();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemsWidget::OnPageShown()
{
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemsWidget::ShowHeaderTabs()
{
	ensureMsgf(RowClass, TEXT("%hs: no RowClass on %s"), __FUNCTION__, *GetName());
	if (ClassTabBox)
	{
		ClassTabBox->SetVisibility(PageSwitcher->GetActiveWidget() == LoadoutPage
									   ? ESlateVisibility::SelfHitTestInvisible
									   : ESlateVisibility::Collapsed);
	}
	if (PageTabBox && RowClass)
	{
		PageTabBox->ClearChildren();
		for (int32 PageIndex = 0; PageIndex < PageNames.Num() && PageIndex < PageSwitcher->GetNumWidgets(); ++PageIndex)
		{
			UGeoListRowWidget* Tab = CreateWidget<UGeoListRowWidget>(this, RowClass);
			Tab->SetTint(PageIndex == PageSwitcher->GetActiveWidgetIndex() ? EGeoListRowTint::Selected
																		   : EGeoListRowTint::Normal);
			Tab->SetSelectable(true);
			Tab->AddTextColumn(PageNames[PageIndex], 0.f);
			Tab->OnClicked.AddWeakLambda(this,
										 [this, PageIndex]
										 {
											 PageSwitcher->SetActiveWidgetIndex(PageIndex);
											 Refresh();
											 ShowHeaderTabs();
										 });
			PageTabBox->AddChildToHorizontalBox(Tab)->SetPadding(FMargin(0.f, 0.f, PageTabGap, 0.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemsWidget::ShowShards()
{
	UGeoGemSubsystem const* GemSubsystem = ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(GetOwningLocalPlayer());
	UGeoGemProfileSave const* Profile = GemSubsystem ? GemSubsystem->GetProfile() : nullptr;
	if (ShardsText && Profile)
	{
		ShardsText->SetText(FText::AsNumber(Profile->GetShards()));
	}
}
