// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoListPanelWidget.h"
#include "OnlineSessionSettings.h"

#include "GeoBrowseServersWidget.generated.h"

class FOnlineSessionSearch;
class UComboBoxString;
class UEditableTextBox;
class UGeoListRowWidget;
class UGeoMenuButton;
class UProgressBar;

/**
 * Browse-servers panel. Finds and lists online sessions; allows filtering by name (client-side)
 * and language (server-side query). Wears the shared list frame and lists into it, so it and the leaderboard are the
 * same list — see UGeoListPanelWidget for the frame, the row class and the OnClosed delegate it closes through.
 * Blueprint subclasses build the header controls and configure data through EditAnywhere properties.
 * Required in the BP hierarchy, on top of the base's: UEditableTextBox "SearchInput", UComboBoxString
 * "LanguageComboBox", UProgressBar "SearchProgressBar", UGeoMenuButton "RefreshButton".
 */
UCLASS()
class GEOTRINITYUI_API UGeoBrowseServersWidget : public UGeoListPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoServer")
	TArray<FString> LanguageOptions;

	/** Clears the list and searches Steam for GeoTrinity lobbies. Run on every open of the panel, not on construct: the
	 * panel is built with the main menu, long before anyone may have hosted. Ignored while a search is in flight. */
	UFUNCTION()
	void FindSessions();

protected:
	/** Populates the language combo box and wires button and text-change delegates. */
	virtual void NativeConstruct() override;
	/** Removes the find-sessions delegate handle to avoid stale callbacks after the widget is destroyed. */
	virtual void NativeDestruct() override;
	/** Returns RefreshButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SearchInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> LanguageComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> SearchProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> RefreshButton;

private:
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle FindSessionsDelegateHandle;
	TArray<FOnlineSessionSearchResult> CachedResults;

	void OnFindSessionsComplete(bool bWasSuccessful);
	void PopulateServerList();
	/** Fills RowWidget with the columns of one session: name, map, players, ping. */
	void FillServerRow(UGeoListRowWidget* RowWidget, const FOnlineSessionSearchResult& Result);
	/** Joins the session the clicked row carried as its payload; taken by value, as a delegate payload must be. */
	void HandleServerSelected(FOnlineSessionSearchResult Result);
	void SetSearchInProgress(bool bInProgress);

	UFUNCTION()
	void HandleSearchTextChanged(const FText& Text);
};
