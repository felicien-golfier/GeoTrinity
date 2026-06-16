// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoBrowseServersWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "GameClasses/GeoGameInstance.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoServerRowWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystemUtils/Classes/FindSessionsCallbackProxy.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SearchInput)
	{
		ensureMsgf(SearchInput, TEXT("UGeoBrowseServersWidget: SearchInput is not bound"));
		return;
	}
	if (!LanguageComboBox)
	{
		ensureMsgf(LanguageComboBox, TEXT("UGeoBrowseServersWidget: LanguageComboBox is not bound"));
		return;
	}
	if (!SearchProgressBar)
	{
		ensureMsgf(SearchProgressBar, TEXT("UGeoBrowseServersWidget: SearchProgressBar is not bound"));
		return;
	}
	if (!RefreshButton)
	{
		ensureMsgf(RefreshButton, TEXT("UGeoBrowseServersWidget: RefreshButton is not bound"));
		return;
	}
	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoBrowseServersWidget: BackButton is not bound"));
		return;
	}
	if (!ServerListScrollBox)
	{
		ensureMsgf(ServerListScrollBox, TEXT("UGeoBrowseServersWidget: ServerListScrollBox is not bound"));
		return;
	}

	LanguageComboBox->ClearOptions();
	LanguageComboBox->AddOption(TEXT("All"));
	for (const FString& Language : LanguageOptions)
	{
		LanguageComboBox->AddOption(Language);
	}
	LanguageComboBox->SetSelectedIndex(0);

	RefreshButton->OnClicked.AddDynamic(this, &UGeoBrowseServersWidget::HandleRefresh);
	BackButton->OnClicked.AddDynamic(this, &UGeoBrowseServersWidget::HandleBack);
	SearchInput->OnTextChanged.AddDynamic(this, &UGeoBrowseServersWidget::HandleSearchTextChanged);

	SearchProgressBar->SetVisibility(ESlateVisibility::Hidden);

	StartFindSessions();
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
void UGeoBrowseServersWidget::PopulateListFromBP(TArray<FBlueprintSessionResult> const& ListOfResults)
{
	UE_LOG(LogTemp, Log, TEXT("Populating list from BP with %u results"), ListOfResults.Num());
	CachedResults.Empty();
	for (const FBlueprintSessionResult& Result : ListOfResults)
	{
		UE_LOG(LogTemp, Log, TEXT("Adding session by %s to CachedResults"), *Result.OnlineResult.Session.OwningUserName);
		
		CachedResults.Add(Result.OnlineResult);
	}
	
	PopulateServerList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::StartFindSessions()
{
	BP_FindSessions();
	
	//Code_FindSessions();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::Code_FindSessions()
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoBrowseServersWidget: Online subsystem not available"));
		return;
	}
	IOnlineSessionPtr Sessions = OnlineSubsystem->GetSessionInterface();
	if (!Sessions.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoBrowseServersWidget: Session interface not valid"));
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->bIsLanQuery = false;
	
	const FString SelectedLanguage = LanguageComboBox->GetSelectedOption();
	if (!SelectedLanguage.IsEmpty() && SelectedLanguage != TEXT("All"))
	{
		SessionSearch->QuerySettings.Set(FName("LANGUAGE"), SelectedLanguage, EOnlineComparisonOp::Equals);
	}
	FindSessionsDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGeoBrowseServersWidget::OnFindSessionsComplete)
	);

	SetSearchInProgress(true);
	
	// User ID
	FUniqueNetIdPtr UserId;
	APlayerState const* PlayerState = GetOwningPlayer()->GetPlayerState<APlayerState>();
	if (PlayerState)
	{
		UserId = PlayerState->GetUniqueId().GetUniqueNetId();
	}
	
	if (UserId.IsValid())
	{
		Sessions->FindSessions(*UserId, SessionSearch.ToSharedRef());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoBrowseServersWidget: Could not get user ID"));
		Sessions->FindSessions(0, SessionSearch.ToSharedRef());
	}
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

	uint32 CodeHash = FNetworkVersion::GetLocalNetworkVersion();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(
		TEXT("[%u] OnFindSessionsComplete complete, found %u results"), CodeHash, SessionSearch->SearchResults.Num()));
	
	CachedResults = SessionSearch->SearchResults;
	PopulateServerList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::PopulateServerList()
{
	UE_LOG(LogTemp, Log, TEXT("Populating Server list with %u results"), CachedResults.Num());
	ServerListScrollBox->ClearChildren();

	if (!ServerRowWidgetClass)
	{
		ensureMsgf(ServerRowWidgetClass, TEXT("UGeoBrowseServersWidget: ServerRowWidgetClass is not set"));
		return;
	}

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

		UGeoServerRowWidget* RowWidget = CreateWidget<UGeoServerRowWidget>(GetOwningPlayer(), ServerRowWidgetClass);
		if (!RowWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("UGeoBrowseServersWidget: Failed to create server row widget"));
			continue;
		}

		RowWidget->InitFromSearchResult(Result);
		RowWidget->OnSelected.AddUObject(this, &UGeoBrowseServersWidget::HandleServerSelected);
		ServerListScrollBox->AddChild(RowWidget);
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(
		TEXT("PopulateServerList, ServerListScrollBox contains %u results"), ServerListScrollBox->GetChildrenCount()));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::HandleServerSelected(const FOnlineSessionSearchResult& Result)
{
	UGeoGameInstance* GeoGameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (!GeoGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoBrowseServersWidget: Could not get GeoGameInstance"));
		return;
	}
	GeoGameInstance->JoinAdvancedSession(Result);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::SetSearchInProgress(bool bInProgress)
{
	SearchProgressBar->SetVisibility(bInProgress ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	RefreshButton->SetIsEnabled(!bInProgress);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::HandleRefresh()
{
	CachedResults.Empty();
	ServerListScrollBox->ClearChildren();
	StartFindSessions();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoBrowseServersWidget::HandleSearchTextChanged(const FText& Text)
{
	PopulateServerList();
}
