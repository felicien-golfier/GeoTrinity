// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoMenuPageWidget.generated.h"

class UGeoMenuButton;
class UGeoMenuPageFrameWidget;
class UGeoMenuRootWidget;

/**
 * Base of every page a menu (UGeoMenuRootWidget) shows in place of its top level. A page never knows who opened it: it
 * opens another page by class (OpenPage) and goes back or closes through its menu, which keeps the path taken — so Back
 * always returns to the page it came from, whichever that was. A page wears PageFrame (Layer 2: the window, Back, the
 * close cross) as the root of its BP tree with its own content in the frame's "PageSlot", and both buttons are wired
 * here; a page that must not offer them wears the bare UGeoMenuFrameWidget instead and leaves PageFrame out. A page
 * shown on its own, outside any menu (the character sheet held open with Tab), hides both: there is nowhere to go.
 * A page lives in its menu's own tree, a direct sibling of every other page of that menu, never inside another page.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoMenuPageWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	/** Runs each time the page comes on top of its menu, opened or uncovered by Back: refresh what may have changed. */
	virtual void OnPageShown() {}

protected:
	/** Wires PageFrame's Back and Close, or hides them outside a menu. */
	virtual void NativeConstruct() override;
	/** Returns the Back button; a page with something better to focus first overrides it. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Goes back to the previous page and consumes the back input. */
	virtual bool HandleBackAction() override;

	/** Shows the page of PageClass in this page's menu, over this one: its Back returns here. */
	void OpenPage(TSubclassOf<UGeoMenuPageWidget> PageClass) const;

	/** False for a page shown on its own, outside any menu. */
	bool IsInMenu() const;

	/** PageFrame's Back button; null on a page wearing no page frame. */
	UGeoMenuButton* GetBackButton() const;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuPageFrameWidget> PageFrame;

private:
	UFUNCTION()
	void GoBack();

	UFUNCTION()
	void CloseMenu();

	/** The menu whose tree holds this page. */
	UGeoMenuRootWidget* GetMenu() const;
};
