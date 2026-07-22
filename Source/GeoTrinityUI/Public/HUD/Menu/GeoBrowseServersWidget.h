// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"
#include "OnlineSessionSettings.h"

#include "GeoBrowseServersWidget.generated.h"

class FOnlineSessionSearch;
class UComboBoxString;
class UEditableTextBox;
class UGeoMenuButton;
class UGeoServerRowWidget;
class UProgressBar;
class UScrollBox;
struct FBlueprintSessionResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoBrowseServersClosedSignature);

/**
 * Browse-servers panel. Finds and lists online sessions; allows filtering by name (client-side)
 * and language (server-side query). Blueprint subclasses build the visual layout and configure
 * data through EditAnywhere properties. Communicates back to the main menu exclusively via the OnClosed
 * delegate, which fires from both BackButton and the panel's back input.
 * Required in the BP hierarchy: UEditableTextBox "SearchInput", UComboBoxString "LanguageComboBox",
 * UProgressBar "SearchProgressBar", UGeoMenuButton "RefreshButton", "BackButton",
 * UScrollBox "ServerListScrollBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoBrowseServersWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Server")
	FGeoBrowseServersClosedSignature OnClosed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
	TSubclassOf<UGeoServerRowWidget> ServerRowWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
	TArray<FString> LanguageOptions;

protected:
	/** Populates the language combo box, wires button and text-change delegates, and triggers an initial session search. */
	virtual void NativeConstruct() override;
	/** Removes the find-sessions delegate handle to avoid stale callbacks after the widget is destroyed. */
	virtual void NativeDestruct() override;
	/** Returns RefreshButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	/** Implemented in Blueprint to trigger the async session search. C++ calls this on widget construct and on refresh. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Server")
	void BP_FindSessions();

	/** Called from Blueprint after BP_FindSessions completes; fills ServerListScrollBox with a row per result. */
	UFUNCTION(BlueprintCallable, Category = "Server")
	void PopulateListFromBP(TArray<FBlueprintSessionResult> const& ListOfResults);
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SearchInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UComboBoxString> LanguageComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> SearchProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UScrollBox> ServerListScrollBox;

private:
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle FindSessionsDelegateHandle;
	TArray<FOnlineSessionSearchResult> CachedResults;

	void StartFindSessions();
	void Code_FindSessions();
	void OnFindSessionsComplete(bool bWasSuccessful);
	void PopulateServerList();
	void HandleServerSelected(const FOnlineSessionSearchResult& Result);
	void SetSearchInProgress(bool bInProgress);

	UFUNCTION()
	void HandleRefresh();

	UFUNCTION()
	void HandleBack();

	UFUNCTION()
	void HandleSearchTextChanged(const FText& Text);
};
