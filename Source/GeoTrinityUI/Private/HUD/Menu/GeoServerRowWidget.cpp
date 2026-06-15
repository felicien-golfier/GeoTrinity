// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoServerRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoServerRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!RowButton)
	{
		ensureMsgf(RowButton, TEXT("UGeoServerRowWidget: RowButton is not bound"));
		return;
	}
	if (!ServerNameText)
	{
		ensureMsgf(ServerNameText, TEXT("UGeoServerRowWidget: ServerNameText is not bound"));
		return;
	}
	if (!MapText)
	{
		ensureMsgf(MapText, TEXT("UGeoServerRowWidget: MapText is not bound"));
		return;
	}
	if (!PlayersText)
	{
		ensureMsgf(PlayersText, TEXT("UGeoServerRowWidget: PlayersText is not bound"));
		return;
	}
	if (!PingText)
	{
		ensureMsgf(PingText, TEXT("UGeoServerRowWidget: PingText is not bound"));
		return;
	}

	RowButton->OnClicked.AddDynamic(this, &UGeoServerRowWidget::HandleRowClicked);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoServerRowWidget::InitFromSearchResult(const FOnlineSessionSearchResult& Result)
{
	StoredResult = Result;

	FString ServerName;
	if (!Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName))
	{
		ServerName = Result.Session.OwningUserName;
	}
	ServerNameText->SetText(FText::FromString(ServerName));

	FString MapName;
	if (!Result.Session.SessionSettings.Get(FName("MAP"), MapName))
	{
		MapName = TEXT("—");
	}
	MapText->SetText(FText::FromString(MapName));

	const int32 MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
	const int32 CurrentPlayers = MaxPlayers - Result.Session.NumOpenPublicConnections;
	PlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentPlayers, MaxPlayers)));

	const FString PingString = (Result.PingInMs < MAX_QUERY_PING)
		? FString::Printf(TEXT("%dms"), Result.PingInMs)
		: TEXT("—");
	PingText->SetText(FText::FromString(PingString));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoServerRowWidget::HandleRowClicked()
{
	OnSelected.Broadcast(StoredResult);
}
