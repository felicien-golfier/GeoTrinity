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

	//CreateButton->OnClicked.AddDynamic(this, &UGeoCreateServerWidget::HandleCreate);
	BackButton->OnClicked.AddDynamic(this, &UGeoCreateServerWidget::HandleBack);
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

	const int32 SelectedMapIndex = MapComboBox->GetSelectedIndex();

	const FText ServerNameText = ServerNameInput->GetText();
	const FString ServerName = ServerNameText.IsEmpty() ? DefaultServerName : ServerNameText.ToString();

	const FString SlotString = SlotsComboBox->GetSelectedOption();
	const int32 NumSlots = SlotString.IsEmpty() ? 2 : FCString::Atoi(*SlotString);
	const FString Language = LanguageComboBox->GetSelectedOption();
	const bool bIsPublic = PrivacyComboBox->GetSelectedOption() == TEXT("Public");

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = NumSlots;	// Should be at least 2 for listen server, according to doc on "CreateAdvancedSession" from Advanced session plugin
	SessionSettings.NumPrivateConnections = bIsPublic ? 0 : NumSlots;
	SessionSettings.bShouldAdvertise = bIsPublic;
	SessionSettings.bAllowJoinInProgress = false;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName("LANGUAGE"), Language, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName("MAP"), MapComboBox->GetSelectedOption(), EOnlineDataAdvertisementType::ViaOnlineService);

	UGeoGameInstance* GeoGameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (!GeoGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UGeoCreateServerWidget: Could not get GeoGameInstance"));
		return;
	}
	
	// Testing the BP function
	GeoGameInstance->BP_CreateAdvancedSession(ServerName, NumSlots, bIsPublic);
	
	// No maps for now, just go to the default one, set in game instance
	//GeoGameInstance->CreateAdvancedSession(SessionSettings);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCreateServerWidget::HandleBack()
{
	OnClosed.Broadcast();
}