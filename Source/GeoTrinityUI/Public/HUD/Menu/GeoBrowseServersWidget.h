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
struct FBlueprintSessionResult;

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

protected:
	/** Populates the language combo box, wires button and text-change delegates, and triggers an initial session search. */
	virtual void NativeConstruct() override;
	/** Removes the find-sessions delegate handle to avoid stale callbacks after the widget is destroyed. */
	virtual void NativeDestruct() override;
	/** Returns RefreshButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

	/** Implemented in Blueprint to trigger the async session search. C++ calls this on widget construct and on refresh. */
	UFUNCTION(BlueprintImplementableEvent, Category = "GeoServer")
	void BP_FindSessions();

	/** Called from Blueprint after BP_FindSessions completes; fills the list with a row per result. */
	UFUNCTION(BlueprintCallable, Category = "GeoServer")
	void PopulateListFromBP(TArray<FBlueprintSessionResult> const& ListOfResults);

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

	void StartFindSessions();
	void Code_FindSessions();
	void OnFindSessionsComplete(bool bWasSuccessful);
	void PopulateServerList();
	/** Fills RowWidget with the columns of one session: name, map, players, ping. */
	void FillServerRow(UGeoListRowWidget* RowWidget, const FOnlineSessionSearchResult& Result);
	/** Joins the session the clicked row carried as its payload; taken by value, as a delegate payload must be. */
	void HandleServerSelected(FOnlineSessionSearchResult Result);
	void SetSearchInProgress(bool bInProgress);

	UFUNCTION()
	void HandleRefresh();

	UFUNCTION()
	void HandleSearchTextChanged(const FText& Text);
};
