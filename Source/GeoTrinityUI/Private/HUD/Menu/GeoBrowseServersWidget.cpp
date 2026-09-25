// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoBrowseServersWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "GameClasses/GeoGameInstance.h"
#include "HUD/Menu/GeoListFrameWidget.h"
#include "HUD/Menu/GeoListRowWidget.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

// Share of the row each column takes, so every row lines up whatever the panel is wide.
static float const ServerNameColumnWeight = 6.f;
static float const MapColumnWeight = 4.f;
static float const PlayersColumnWeight = 2.f;
static float const PingColumnWeight = 2.f;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LanguageComboBox->ClearOptions();
	LanguageComboBox->AddOption(TEXT("All"));
	for (const FString& Language : LanguageOptions)
	{
		LanguageComboBox->AddOption(Language);
	}
	LanguageComboBox->SetSelectedIndex(0);

	RefreshButton->OnClicked.AddUniqueDynamic(this, &UGeoBrowseServersWidget::FindSessions);
	SearchInput->OnTextChanged.AddUniqueDynamic(this, &UGeoBrowseServersWidget::HandleSearchTextChanged);

	SearchProgressBar->SetVisibility(ESlateVisibility::Hidden);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::NativeDestruct()
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem && FindSessionsDelegateHandle.IsValid())
	{
		IOnlineSessionPtr Sessions = OnlineSubsystem->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		}
	}

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoBrowseServersWidget::GetInitialFocusWidget() const
{
	return RefreshButton;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::FindSessions()
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!ensureMsgf(Sessions.IsValid(), TEXT("%hs: no online session interface"), __FUNCTION__))
	{
		return;
	}

	if (SessionSearch.IsValid() && SessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		UE_LOG(LogTemp, Log, TEXT("%hs: a search is already in flight"), __FUNCTION__);
		return;
	}

	CachedResults.Empty();
	ListFrame->RowsBox->ClearChildren();

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(UGeoGameInstance::GameSessionKey, 1, EOnlineComparisonOp::Equals);

	FString const SelectedLanguage = LanguageComboBox->GetSelectedOption();
	if (!SelectedLanguage.IsEmpty() && SelectedLanguage != TEXT("All"))
	{
		SessionSearch->QuerySettings.Set(FName("LANGUAGE"), SelectedLanguage, EOnlineComparisonOp::Equals);
	}

	FindSessionsDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGeoBrowseServersWidget::OnFindSessionsComplete));
	SetSearchInProgress(true);
	Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr Sessions = OnlineSubsystem->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		}
	}

	SetSearchInProgress(false);

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoBrowseServersWidget: FindSessions failed"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%hs: network version %u, found %d results"), __FUNCTION__,
		   FNetworkVersion::GetLocalNetworkVersion(), SessionSearch->SearchResults.Num());

	CachedResults = SessionSearch->SearchResults;
	PopulateServerList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::PopulateServerList()
{
	UE_LOG(LogTemp, Log, TEXT("Populating Server list with %u results"), CachedResults.Num());
	ListFrame->RowsBox->ClearChildren();

	const FString FilterText = SearchInput->GetText().ToString().ToLower();

	for (const FOnlineSessionSearchResult& Result : CachedResults)
	{
		if (!FilterText.IsEmpty())
		{
			FString ServerName;
			Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);
			if (!ServerName.ToLower().Contains(FilterText))
			{
				UE_LOG(LogTemp, Log, TEXT("Server %s was excluded by filter %s"), *ServerName, *FilterText);
				continue;
			}
		}

		UGeoListRowWidget* const RowWidget = MakeRow();
		if (!RowWidget)
		{
			return;
		}

		RowWidget->SetTint(ListFrame->RowsBox->GetChildrenCount() % 2 == 0 ? EGeoListRowTint::Normal
																		 : EGeoListRowTint::Alternate);
		FillServerRow(RowWidget, Result);
		// The row carries the session it stands for as the payload of its click, so the row itself holds no state.
		RowWidget->OnClicked.AddUObject(this, &UGeoBrowseServersWidget::HandleServerSelected, Result);
		ListFrame->RowsBox->AddChild(RowWidget);
	}

	UE_LOG(LogTemp, Log, TEXT("%hs: listing %d server rows"), __FUNCTION__,
		   ListFrame->RowsBox->GetChildrenCount());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::FillServerRow(UGeoListRowWidget* RowWidget, const FOnlineSessionSearchResult& Result)
{
	FString ServerName;
	if (!Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName))
	{
		ServerName = Result.Session.OwningUserName;
	}
	RowWidget->AddTextColumn(FText::FromString(ServerName), ServerNameColumnWeight);

	FString MapName;
	if (!Result.Session.SessionSettings.Get(FName("MAP"), MapName))
	{
		MapName = TEXT("-");
	}
	RowWidget->AddTextColumn(FText::FromString(MapName), MapColumnWeight);

	const int32 MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
	const int32 CurrentPlayers = MaxPlayers - Result.Session.NumOpenPublicConnections;
	RowWidget->AddTextColumn(FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentPlayers, MaxPlayers)),
							 PlayersColumnWeight);

	const FString PingString =
		(Result.PingInMs < MAX_QUERY_PING) ? FString::Printf(TEXT("%dms"), Result.PingInMs) : TEXT("-");
	RowWidget->AddTextColumn(FText::FromString(PingString), PingColumnWeight);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::HandleServerSelected(FOnlineSessionSearchResult Result)
{
	UGeoGameInstance* GeoGameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (!GeoGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoBrowseServersWidget: Could not get GeoGameInstance"));
		return;
	}
	GeoGameInstance->JoinFoundSession(Result);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::SetSearchInProgress(bool bInProgress)
{
	SearchProgressBar->SetVisibility(bInProgress ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	RefreshButton->SetIsEnabled(!bInProgress);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::HandleSearchTextChanged(const FText& Text)
{
	PopulateServerList();
}
