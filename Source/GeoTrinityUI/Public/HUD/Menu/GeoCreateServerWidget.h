// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoCreateServerWidget.generated.h"

class UComboBoxString;
class UEditableTextBox;
class UGeoMenuButton;

/**
 * "Create Server" form widget. Reads server settings from its form fields and creates a session.
 * Blueprint subclasses build the visual layout and set the data arrays (MapDisplayNames, MapURLs, etc.).
 * Required in the BP hierarchy: UEditableTextBox "ServerNameInput", UComboBoxString "MapComboBox",
 * "SlotsComboBox", "LanguageComboBox", "PrivacyComboBox", UGeoMenuButton "CreateButton".
 */
UCLASS()
class GEOTRINITYUI_API UGeoCreateServerWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoServer|Maps")
	TArray<FString> MapDisplayNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoServer|Maps")
	TArray<FString> MapURLs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoServer")
	TArray<int32> SlotOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoServer")
	TArray<FString> LanguageOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoServer")
	FString DefaultServerName = TEXT("My Server");

protected:
	/** Populates the combo boxes from the data arrays, resets the server name to DefaultServerName, and wires button delegates. */
	virtual void NativeConstruct() override;
	/** Returns CreateButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

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

private:
	UFUNCTION()
	void HandleCreate();

	void PopulateComboBoxes();
};
