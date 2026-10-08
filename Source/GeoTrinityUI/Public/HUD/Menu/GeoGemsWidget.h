// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoGemsWidget.generated.h"

class UGeoGemForgeWidget;
class UGeoGemLoadoutWidget;
class UGeoListRowWidget;
class UGeoMenuButton;
class UHorizontalBox;
class UTextBlock;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoGemsClosedSignature);

/**
 * The Gems menu: the LOADOUT and FORGE pages under one header holding their tabs and the shards owned. Opened from the
 * character sheet; BACK, or the back input, closes it.
 * Required in the BP hierarchy: UWidgetSwitcher "PageSwitcher" holding UGeoGemLoadoutWidget "LoadoutPage" then
 * UGeoGemForgeWidget "ForgePage", and UGeoMenuButton "BackButton". Optional: UHorizontalBox "PageTabBox", UTextBlock
 * "ShardsText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	/** Shows both pages and the shards as the profile is now, as each open of the menu does. */
	void Refresh();

	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoGemsClosedSignature OnClosed;

protected:
	/** Follows the pages' changes to the profile with the shard count, once for the widget's life. */
	virtual void NativeOnInitialized() override;
	/** Wires BACK and shows the page tabs. */
	virtual void NativeConstruct() override;
	virtual UWidget* GetInitialFocusWidget() const override;
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> PageTabBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShardsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoGemLoadoutWidget> LoadoutPage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoGemForgeWidget> ForgePage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	/** The page tabs: WBP_ListRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TSubclassOf<UGeoListRowWidget> RowClass;

	/** One per page of PageSwitcher, in its order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TArray<FText> PageNames = {INVTEXT("LOADOUT"), INVTEXT("FORGE")};

	/** Space between two page tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0", ClampMax = "64"))
	float PageTabGap = 10.f;

private:
	UFUNCTION()
	void HandleBack();

	void ShowPageTabs();
	void ShowShards();
};
