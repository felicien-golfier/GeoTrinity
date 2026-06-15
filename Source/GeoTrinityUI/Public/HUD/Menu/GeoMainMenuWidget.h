// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoMainMenuWidget.generated.h"

class UGeoCreateServerWidget;
class UGeoBrowseServersWidget;
class UGeoLocalConnectWidget;
class UGeoMenuButton;

/**
 * Main lobby menu widget. Composes the UGeoMenuButton instances and handles all action logic in C++.
 * Blueprint subclasses configure appearance through the button UPROPERTYs.
 * Required in the BP hierarchy: UGeoMenuButton widgets named "CreateServerButton", "JoinServerButton",
 * "PlayLocalButton", "QuitButton", and panel widgets "CreateServerWidget", "BrowseServerWidget", "LocalConnectWidget"
 * (panels set Collapsed by default in the BP layout).
 */
UCLASS()
class GEOTRINITYUI_API UGeoMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Returns the local player's display name from the game instance settings; used to populate name labels in the lobby. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	FString GetLocalPlayerName() const;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> CreateServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> JoinServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> PlayLocalButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoCreateServerWidget> CreateServerWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoBrowseServersWidget> BrowseServerWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoLocalConnectWidget> LocalConnectWidget;

private:
	UFUNCTION()
	void HandleCreateServer();

	UFUNCTION()
	void HandleJoinServer();

	UFUNCTION()
	void HandlePlayLocal();

	UFUNCTION()
	void HandleQuit();

	UFUNCTION()
	void HandleCreateServerClosed();

	UFUNCTION()
	void HandleBrowseServerClosed();

	UFUNCTION()
	void HandleLocalConnectClosed();

	void SetButtonsVisible(bool bVisible);
};
