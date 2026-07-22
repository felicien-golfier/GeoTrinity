// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoCreateServerWidget.generated.h"

class UComboBoxString;
class UEditableTextBox;
class UGeoMenuButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoCreateServerClosedSignature);

/**
 * "Create Server" form widget. Reads server settings from its form fields and creates a session.
 * Blueprint subclasses build the visual layout and set the data arrays (MapDisplayNames, MapURLs, etc.).
 * Communicates back to the main menu exclusively via the OnClosed delegate — no hard upward reference — which
 * fires from both BackButton and the panel's back input.
 * Required in the BP hierarchy: UEditableTextBox "ServerNameInput", UComboBoxString "MapComboBox",
 * "SlotsComboBox", "LanguageComboBox", "PrivacyComboBox", UGeoMenuButton "CreateButton", "BackButton".
 */
UCLASS()
class GEOTRINITYUI_API UGeoCreateServerWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Server")
	FGeoCreateServerClosedSignature OnClosed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server|Maps")
	TArray<FString> MapDisplayNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server|Maps")
	TArray<FString> MapURLs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
	TArray<int32> SlotOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
	TArray<FString> LanguageOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
	FString DefaultServerName = TEXT("My Server");

protected:
	/** Populates the combo boxes from the data arrays, resets the server name to DefaultServerName, and wires button delegates. */
	virtual void NativeConstruct() override;
	/** Returns CreateButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ServerNameInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> MapComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> SlotsComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> LanguageComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> PrivacyComboBox;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> CreateButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

private:
	UFUNCTION()
	void HandleCreate();

	UFUNCTION()
	void HandleBack();

	void PopulateComboBoxes();

	FString PendingMapURL;
};
