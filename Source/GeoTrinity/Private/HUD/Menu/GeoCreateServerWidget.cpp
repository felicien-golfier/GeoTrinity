// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoCreateServerWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ServerNameInput)
	{
		ensureMsgf(ServerNameInput, TEXT("UGeoCreateServerWidget: ServerNameInput is not bound"));
		return;
	}
	if (!MapComboBox)
	{
		ensureMsgf(MapComboBox, TEXT("UGeoCreateServerWidget: MapComboBox is not bound"));
		return;
	}
	if (!SlotsComboBox)
	{
		ensureMsgf(SlotsComboBox, TEXT("UGeoCreateServerWidget: SlotsComboBox is not bound"));
		return;
	}
	if (!LanguageComboBox)
	{
		ensureMsgf(LanguageComboBox, TEXT("UGeoCreateServerWidget: LanguageComboBox is not bound"));
		return;
	}
	if (!PrivacyComboBox)
	{
		ensureMsgf(PrivacyComboBox, TEXT("UGeoCreateServerWidget: PrivacyComboBox is not bound"));
		return;
	}
	if (!CreateButton)
	{
		ensureMsgf(CreateButton, TEXT("UGeoCreateServerWidget: CreateButton is not bound"));
		return;
	}
	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoCreateServerWidget: BackButton is not bound"));
		return;
	}

	PopulateComboBoxes();
	ServerNameInput->SetHintText(FText::FromString(DefaultServerName));

	CreateButton->OnClicked.AddDynamic(this, &UGeoCreateServerWidget::HandleCreate);
	BackButton->OnClicked.AddDynamic(this, &UGeoCreateServerWidget::HandleBack);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::PopulateComboBoxes()
{
	MapComboBox->ClearOptions();
	for (const FString& MapName : MapDisplayNames)
	{
		MapComboBox->AddOption(MapName);
	}
	if (MapComboBox->GetOptionCount() > 0)
	{
		MapComboBox->SetSelectedIndex(0);
	}

	SlotsComboBox->ClearOptions();
	for (int32 Slots : SlotOptions)
	{
		SlotsComboBox->AddOption(FString::FromInt(Slots));
	}
	if (SlotsComboBox->GetOptionCount() > 0)
	{
		SlotsComboBox->SetSelectedIndex(0);
	}

	LanguageComboBox->ClearOptions();
	for (const FString& Language : LanguageOptions)
	{
		LanguageComboBox->AddOption(Language);
	}
	if (LanguageComboBox->GetOptionCount() > 0)
	{
		LanguageComboBox->SetSelectedIndex(0);
	}

	PrivacyComboBox->ClearOptions();
	PrivacyComboBox->AddOption(TEXT("Public"));
	PrivacyComboBox->AddOption(TEXT("Private"));
	PrivacyComboBox->SetSelectedIndex(0);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::HandleCreate()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!ensureMsgf(OnlineSub, TEXT("UGeoCreateServerWidget: Online subsystem not available")))
	{
		return;
	}

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (!ensureMsgf(Sessions.IsValid(), TEXT("UGeoCreateServerWidget: Session interface not valid")))
	{
		return;
	}

	const int32 SelectedMapIndex = MapComboBox->GetSelectedIndex();
	if (!MapURLs.IsValidIndex(SelectedMapIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoCreateServerWidget: No URL configured for map at index %d — check MapURLs array"), SelectedMapIndex);
		return;
	}

	PendingMapURL = MapURLs[SelectedMapIndex];

	const FText ServerNameText = ServerNameInput->GetText();
	const FString ServerName = ServerNameText.IsEmpty() ? DefaultServerName : ServerNameText.ToString();

	const FString SlotString = SlotsComboBox->GetSelectedOption();
	const int32 NumSlots = SlotString.IsEmpty() ? 2 : FCString::Atoi(*SlotString);
	const FString Language = LanguageComboBox->GetSelectedOption();
	const bool bIsPublic = PrivacyComboBox->GetSelectedOption() == TEXT("Public");

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = NumSlots;
	SessionSettings.NumPrivateConnections = bIsPublic ? 0 : NumSlots;
	SessionSettings.bShouldAdvertise = bIsPublic;
	SessionSettings.bAllowJoinInProgress = false;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName("LANGUAGE"), Language, EOnlineDataAdvertisementType::ViaOnlineService);

	CreateSessionDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGeoCreateServerWidget::OnCreateSessionComplete)
	);

	Sessions->CreateSession(0, FName(TEXT("GameSession")), SessionSettings);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
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
		UE_LOG(LogTemp, Error, TEXT("UGeoCreateServerWidget: Failed to create session '%s'"), *SessionName.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UGeoCreateServerWidget: Session created, traveling to %s"), *PendingMapURL);
	GetWorld()->ServerTravel(PendingMapURL + TEXT("?listen"));
}
