// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoMainMenuWidget.generated.h"

class UGeoMenuButton;
class UGeoCreateServerWidget;

/**
 * Main lobby menu widget. Composes three UGeoMenuButton instances and handles all action logic in C++.
 * Blueprint subclasses configure appearance through the button UPROPERTYs.
 * Required in the BP hierarchy: UGeoMenuButton widgets named "CreateServerButton", "JoinServerButton", "QuitButton",
 * and a UGeoCreateServerWidget named "CreateServerWidget" (set Collapsed by default in the BP layout).
 */
UCLASS()
class GEOTRINITY_API UGeoMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Session")
	FString GetLocalPlayerName() const;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> CreateServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> JoinServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoCreateServerWidget> CreateServerWidget;

private:
	UFUNCTION()
	void HandleCreateServer();

	UFUNCTION()
	void HandleJoinServer();

	UFUNCTION()
	void HandleQuit();

	UFUNCTION()
	void HandleCreateServerClosed();

	void SetButtonsVisible(bool bVisible);
};
