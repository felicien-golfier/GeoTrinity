// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoAbilityDescriptionsWidget.generated.h"

class UGeoAbilityCardWidget;
class UGeoAbilityDetailWidget;
class UGeoMenuButton;
class UGeoShape;
class UTextBlock;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoAbilityDescriptionsClosedSignature);

/**
 * Full-screen ability compendium for the local player's class: CardGrid is rebuilt on each open with one CardClass card
 * per catalogued ability (UAbilityInfo::GetAbilitiesForClass), actives first, CardColumns to a row. The header shows the
 * class's shape, name and role from the theme's class style. Picking a card opens its details in DetailWidget, the
 * drawer sliding in from the right over DetailScrim; a click outside the drawer, or Back, closes it. Communicates back to the parent
 * menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", UUniformGridPanel "CardGrid".
 */
UCLASS()
class GEOTRINITYUI_API UGeoAbilityDescriptionsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "GeoMenu")
	FGeoAbilityDescriptionsClosedSignature OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Closes the open details, else the page. */
	virtual bool HandleBackAction() override;
	/** Closes the open details: only a click outside the drawer reaches here while it is open. */
	virtual FReply NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	/** Optional only so the Blueprint compiles before AI/Python/UI/ability_page.py has added it; required to show cards. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> CardGrid;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoAbilityDetailWidget> DetailWidget;

	/** Dims the cards while the details are open, and takes the click that closes them. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DetailScrim;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoShape> ClassShape;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassRoleText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityPage")
	TSubclassOf<UGeoAbilityCardWidget> CardClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoAbilityPage", meta = (ClampMin = "1", ClampMax = "6"))
	int32 CardColumns = 3;

private:
	UFUNCTION()
	void HandleBack();

	void BuildCards();

	UFUNCTION()
	void HandleCardSelected(UGeoAbilityCardWidget* Card);

	void CloseDetail();

	UPROPERTY()
	TArray<TObjectPtr<UGeoAbilityCardWidget>> Cards;

	/** The card whose details are open; INDEX_NONE when closed. */
	int32 SelectedIndex = INDEX_NONE;

	/** Shows PlayerClass's shape, name and role in the header. */
	void ShowClass(EPlayerClass PlayerClass);
};
