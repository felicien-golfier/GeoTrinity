// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"

#include "GeoServerRowWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FGeoServerRowSelectedDelegate, const FOnlineSessionSearchResult&);

/**
 * One entry in the browse-servers list; displays server name, map, player count, and ping.
 * Blueprint subclasses build the visual layout; all logic is in C++.
 * Required in the BP hierarchy: UButton "RowButton", UTextBlock "ServerNameText", "MapText",
 * "PlayersText", "PingText". Optional: UImage "FlagImage".
 */
UCLASS()
class GEOTRINITYUI_API UGeoServerRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FGeoServerRowSelectedDelegate OnSelected;

	void InitFromSearchResult(const FOnlineSessionSearchResult& Result);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> RowButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> ServerNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> MapText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayersText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> PingText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> FlagImage;

private:
	FOnlineSessionSearchResult StoredResult;

	UFUNCTION()
	void HandleRowClicked();
};
