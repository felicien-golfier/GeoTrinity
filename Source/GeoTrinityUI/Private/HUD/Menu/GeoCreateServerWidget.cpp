// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoCreateServerWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameClasses/GeoGameInstance.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PopulateComboBoxes();
	ServerNameInput->SetHintText(FText::FromString(DefaultServerName));

	CreateButton->OnClicked.AddUniqueDynamic(this, &UGeoCreateServerWidget::HandleCreate);
	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoCreateServerWidget::HandleBack);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoCreateServerWidget::GetInitialFocusWidget() const
{
	return CreateButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoCreateServerWidget::HandleBackAction()
{
	HandleBack();
	return true;
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
	UGeoGameInstance* GeoGameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	int32 const MapIndex = MapComboBox->GetSelectedIndex();
	int32 const SlotsIndex = SlotsComboBox->GetSelectedIndex();
	if (!ensureMsgf(GeoGameInstance, TEXT("%hs: GameInstance is not a UGeoGameInstance"), __FUNCTION__)
		|| !ensureMsgf(MapURLs.IsValidIndex(MapIndex), TEXT("%hs: no map URL for map %d"), __FUNCTION__, MapIndex)
		|| !ensureMsgf(SlotOptions.IsValidIndex(SlotsIndex), TEXT("%hs: no slot option selected"), __FUNCTION__))
	{
		return;
	}

	FText const ServerNameText = ServerNameInput->GetText();
	FString const ServerName = ServerNameText.IsEmpty() ? DefaultServerName : ServerNameText.ToString();
	int32 const NumSlots = SlotOptions[SlotsIndex];
	bool const bIsPublic = PrivacyComboBox->GetSelectedOption() == TEXT("Public");

	FOnlineSessionSettings SessionSettings;
	// A private session is an unlisted Steam lobby, reachable only through an invite.
	SessionSettings.NumPublicConnections = bIsPublic ? NumSlots : 0;
	SessionSettings.NumPrivateConnections = bIsPublic ? 0 : NumSlots;
	SessionSettings.bShouldAdvertise = bIsPublic;
	// The session starts with every boss fight (AGameSession::HandleMatchHasStarted), and Steam drops a lobby that
	// refuses joins from every search.
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowInvites = true;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName("LANGUAGE"), LanguageComboBox->GetSelectedOption(),
						EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName("MAP"), MapComboBox->GetSelectedOption(), EOnlineDataAdvertisementType::ViaOnlineService);

	GeoGameInstance->CreateSession(SessionSettings, FSoftObjectPath(MapURLs[MapIndex]).GetLongPackageName());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::HandleBack()
{
	OnClosed.Broadcast();
}