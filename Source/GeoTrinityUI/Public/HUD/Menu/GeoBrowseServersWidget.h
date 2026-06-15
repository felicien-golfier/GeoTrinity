// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
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
 * data through EditAnywhere properties.
 * Required in the BP hierarchy: UEditableTextBox "SearchInput", UComboBoxString "LanguageComboBox",
 * UProgressBar "SearchProgressBar", UGeoMenuButton "RefreshButton", "BackButton",
 * UScrollBox "ServerListScrollBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoBrowseServersWidget : public UUserWidget
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
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
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
