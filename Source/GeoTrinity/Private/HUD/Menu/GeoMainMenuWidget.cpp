// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMainMenuWidget.h"

#include "HUD/Menu/GeoMenuButton.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

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

	CreateServerButton->OnClicked.AddDynamic(this, &UGeoMainMenuWidget::HandleCreateServer);
	JoinServerButton->OnClicked.AddDynamic(this, &UGeoMainMenuWidget::HandleJoinServer);
	QuitButton->OnClicked.AddDynamic(this, &UGeoMainMenuWidget::HandleQuit);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleCreateServer()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!ensureMsgf(OnlineSub, TEXT("UGeoMainMenuWidget: Online subsystem not available")))
	{
		return;
	}

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (!ensureMsgf(Sessions.IsValid(), TEXT("UGeoMainMenuWidget: Session interface not valid")))
	{
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = MaxPublicConnections;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = false;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;

	CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGeoMainMenuWidget::OnCreateSessionComplete)
	);

	Sessions->CreateSession(0, FName(TEXT("GameSession")), SessionSettings);
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
void UGeoMainMenuWidget::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		}
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoMainMenuWidget: Failed to create session '%s'"), *SessionName.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UGeoMainMenuWidget: Session created, traveling to %s"), *GameMapURL);
	GetWorld()->ServerTravel(GameMapURL + TEXT("?listen"));
}
