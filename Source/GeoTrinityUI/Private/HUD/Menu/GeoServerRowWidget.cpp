// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoServerRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoServerRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RowButton->OnClicked.AddUniqueDynamic(this, &UGeoServerRowWidget::HandleRowClicked);
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
