// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoGemsWidget.generated.h"

class UGeoGemForgeWidget;
class UGeoGemLoadoutWidget;
class UGeoListRowWidget;
class UHorizontalBox;
class UTextBlock;
class UWidgetSwitcher;

/**
 * The Gems menu: the LOADOUT and FORGE pages under one header holding their tabs, the Loadout's class tabs while it is
 * the page shown, and the shards owned. Opened from the character sheet.
 * Required in the BP hierarchy: UWidgetSwitcher "PageSwitcher" holding UGeoGemLoadoutWidget "LoadoutPage" then
 * UGeoGemForgeWidget "ForgePage". Optional: UHorizontalBox "PageTabBox", UHorizontalBox "ClassTabBox", UTextBlock
 * "ShardsText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemsWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

public:
	/** Shows both pages and the shards as the profile is now. */
	void Refresh();

	/** Refreshes: the profile may have changed since the menu was last open. */
	virtual void OnPageShown() override;

protected:
	/** Follows the pages' changes to the profile with the shard count, once for the widget's life. */
	virtual void NativeOnInitialized() override;
	/** Shows the header's tabs. */
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> PageTabBox;

	/** Lent to LoadoutPage for its class tabs; collapsed while another page is shown. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> ClassTabBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShardsText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoGemLoadoutWidget> LoadoutPage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoGemForgeWidget> ForgePage;

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
	/** The page tabs, and the class tabs shown while the Loadout is. */
	void ShowHeaderTabs();
	void ShowShards();
};
