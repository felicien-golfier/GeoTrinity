// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMainMenuWidget.h"

#include "HUD/Menu/GeoCreateServerWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/KismetSystemLibrary.h"
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

	if (!CreateServerButton)
	{
		ensureMsgf(CreateServerButton, TEXT("UGeoMainMenuWidget: CreateServerButton is not bound"));
		return;
	}
	if (!JoinServerButton)
	{
		ensureMsgf(JoinServerButton, TEXT("UGeoMainMenuWidget: JoinServerButton is not bound"));
		return;
	}
	if (!QuitButton)
	{
		ensureMsgf(QuitButton, TEXT("UGeoMainMenuWidget: QuitButton is not bound"));
		return;
	}
	if (!CreateServerWidget)
	{
		ensureMsgf(CreateServerWidget, TEXT("UGeoMainMenuWidget: CreateServerWidget is not bound"));
		return;
	}

	CreateServerButton->OnClicked.AddDynamic(this, &UGeoMainMenuWidget::HandleCreateServer);
	JoinServerButton->OnClicked.AddDynamic(this, &UGeoMainMenuWidget::HandleJoinServer);
	QuitButton->OnClicked.AddDynamic(this, &UGeoMainMenuWidget::HandleQuit);
	CreateServerWidget->OnClosed.AddDynamic(this, &UGeoMainMenuWidget::HandleCreateServerClosed);

	CreateServerWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleCreateServer()
{
	SetButtonsVisible(false);
	CreateServerWidget->SetVisibility(ESlateVisibility::Visible);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleJoinServer()
{
	UE_LOG(LogTemp, Warning, TEXT("UGeoMainMenuWidget: Join server not yet implemented"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleCreateServerClosed()
{
	CreateServerWidget->SetVisibility(ESlateVisibility::Collapsed);
	SetButtonsVisible(true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::SetButtonsVisible(bool bVisible)
{
	const ESlateVisibility NewVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	CreateServerButton->SetVisibility(NewVisibility);
	JoinServerButton->SetVisibility(NewVisibility);
	QuitButton->SetVisibility(NewVisibility);
}
