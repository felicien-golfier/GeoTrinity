// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMainMenuWidget.h"

#include "GameClasses/GeoGameInstance.h"
#include "HUD/Menu/GeoBrowseServersWidget.h"
#include "HUD/Menu/GeoCreateServerWidget.h"
#include "HUD/Menu/GeoLeaderboardWidget.h"
#include "HUD/Menu/GeoLocalConnectWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// ---------------------------------------------------------------------------------------------------------------------
FString UGeoMainMenuWidget::GetLocalPlayerName() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGeoMainMenuWidget::GetLocalPlayerName: Online subsystem not available"));
		return FString();
	}

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UGeoMainMenuWidget::GetLocalPlayerName: Identity interface not valid"));
		return FString();
	}

	return Identity->GetPlayerNickname(0);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CreateServerButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleCreateServer);
	JoinServerButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleJoinServer);
	PlayLocalButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandlePlayLocal);
	LeaderboardButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleLeaderboard);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleQuit);
	CreateServerWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);
	BrowseServerWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);
	LocalConnectWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);
	LeaderboardWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);

	CreateServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	BrowseServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	LocalConnectWidget->SetVisibility(ESlateVisibility::Collapsed);
	LeaderboardWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (GameInstance && !ErrorPopup.IsValid())
	{
		FString const SessionError = GameInstance->TakeSessionError();
		if (!SessionError.IsEmpty())
		{
			ShowErrorPopup(SessionError);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::ShowErrorPopup(FString const& Message)
{
	TSharedRef<SButton> const OkButton =
		SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(32.f, 8.f))
			.OnClicked_UObject(this, &UGeoMainMenuWidget::HandleErrorPopupClosed)
				[SNew(STextBlock).Text(INVTEXT("OK")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))];

	ErrorPopup =
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.7f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
				[SNew(SBox)
					 .MaxDesiredWidth(800.f)
						 [SNew(SBorder)
							  .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							  .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.08f, 1.f))
							  .Padding(24.f)
								  [SNew(SVerticalBox)
								   + SVerticalBox::Slot().AutoHeight()
										 [SNew(STextBlock)
											  .Text(INVTEXT("Connection error"))
											  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
											  .ColorAndOpacity(FLinearColor(1.f, 0.3f, 0.3f, 1.f))]
								   + SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f)
										 [SNew(STextBlock)
											  .Text(FText::FromString(Message))
											  .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
											  .AutoWrapText(true)]
								   + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[OkButton]]]];

	GetGameInstance()->GetGameViewportClient()->AddViewportWidgetContent(ErrorPopup.ToSharedRef(), 100);
	FSlateApplication::Get().SetAllUserFocus(OkButton, EFocusCause::SetDirectly);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMainMenuWidget::HandleErrorPopupClosed()
{
	GetGameInstance()->GetGameViewportClient()->RemoveViewportWidgetContent(ErrorPopup.ToSharedRef());
	ErrorPopup.Reset();
	SetFocus();
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoMainMenuWidget::GetInitialFocusWidget() const
{
	return CreateServerButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoMainMenuWidget::HandleEscapeAction()
{
	HandleSubPanelClosed();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleCreateServer()
{
	OpenSubPanel(CreateServerWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleJoinServer()
{
	OpenSubPanel(BrowseServerWidget);
	BrowseServerWidget->FindSessions();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandlePlayLocal()
{
	OpenSubPanel(LocalConnectWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleLeaderboard()
{
	OpenSubPanel(LeaderboardWidget);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleQuit()
{
	UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (!ensureMsgf(GameInstance, TEXT("UGeoMainMenuWidget::HandleQuit: GameInstance is not a UGeoGameInstance")))
	{
		return;
	}
	GameInstance->QuitGame();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleSubPanelClosed()
{
	CreateServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	BrowseServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	LocalConnectWidget->SetVisibility(ESlateVisibility::Collapsed);
	LeaderboardWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
	CreateServerButton->SetFocus();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::OpenSubPanel(UGeoMenuPanelWidget* SubPanel)
{
	SetButtonsVisible(false);
	SubPanel->SetVisibility(ESlateVisibility::Visible);
	SubPanel->SetFocus();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::SetButtonsVisible(bool bVisible)
{
	const ESlateVisibility NewVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	CreateServerButton->SetVisibility(NewVisibility);
	JoinServerButton->SetVisibility(NewVisibility);
	PlayLocalButton->SetVisibility(NewVisibility);
	LeaderboardButton->SetVisibility(NewVisibility);
	QuitButton->SetVisibility(NewVisibility);
}
