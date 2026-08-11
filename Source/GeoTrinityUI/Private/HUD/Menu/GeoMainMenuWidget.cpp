// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMainMenuWidget.h"

#include "GameClasses/GeoGameInstance.h"
#include "HUD/Menu/GeoBrowseServersWidget.h"
#include "HUD/Menu/GeoCreateServerWidget.h"
#include "HUD/Menu/GeoLocalConnectWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"

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
	QuitButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleQuit);
	CreateServerWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);
	BrowseServerWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);
	LocalConnectWidget->OnClosed.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleSubPanelClosed);

	CreateServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	BrowseServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	LocalConnectWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoMainMenuWidget::GetInitialFocusWidget() const
{
	return CreateServerButton;
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
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandlePlayLocal()
{
	OpenSubPanel(LocalConnectWidget);
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
	QuitButton->SetVisibility(NewVisibility);
}
