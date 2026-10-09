// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuRootWidget.h"

#include "GeoMainMenuWidget.generated.h"

class UGeoMenuButton;

/**
 * Main lobby menu widget. Composes the UGeoMenuButton instances and handles all action logic in C++.
 * Blueprint subclasses configure appearance through the button UPROPERTYs.
 * Required in the BP hierarchy: UGeoMenuButton widgets named "CreateServerButton", "JoinServerButton",
 * "PlayLocalButton", "LeaderboardButton", "QuitButton", and as pages a UGeoCreateServerWidget, a
 * UGeoBrowseServersWidget, a UGeoLocalConnectWidget and a UGeoLeaderboardWidget.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMainMenuWidget : public UGeoMenuRootWidget
{
	GENERATED_BODY()

public:
	/** Returns the local player's display name from the game instance settings; used to populate name labels in the lobby. */
	UFUNCTION(BlueprintCallable, Category = "GeoSession")
	FString GetLocalPlayerName() const;

protected:
	/** Wires each main-menu button to its handler. */
	virtual void NativeConstruct() override;
	/** Shows the error popup for any create/join/connection failure the game instance holds (TakeSessionError). */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;
	/** Returns CreateServerButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> CreateServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> JoinServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> PlayLocalButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> LeaderboardButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

private:
	UFUNCTION()
	void HandleCreateServer();

	UFUNCTION()
	void HandleJoinServer();

	UFUNCTION()
	void HandlePlayLocal();

	UFUNCTION()
	void HandleLeaderboard();

	UFUNCTION()
	void HandleQuit();

	/** Pure Slate modal over the whole viewport, so it needs nothing from the menu's BP layout; its OK button takes
	 * every user's focus so a gamepad can dismiss it. */
	void ShowErrorPopup(FString const& Message);
	FReply HandleErrorPopupClosed();

	TSharedPtr<SWidget> ErrorPopup;
};
