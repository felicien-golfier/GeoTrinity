// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoMenuRootWidget.generated.h"

class UGeoMenuPageWidget;

/**
 * A menu the player navigates through pages: its TopLevel (its own buttons and dressing), and every UGeoMenuPageWidget
 * in its tree, one of which shows at a time in TopLevel's place. Pages are reached by class, from TopLevel or from
 * another page, and the menu keeps the path taken, so Back retraces it page by page and Close leaves every page at once.
 * Base of UGeoMainMenuWidget and UGeoPauseMenuWidget.
 * Required in the BP hierarchy: a widget "TopLevel" holding everything shown while no page is open.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoMenuRootWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	/** Shows the page of PageClass held in this menu's tree over whatever shows now, which Back then returns to. */
	void OpenPage(TSubclassOf<UGeoMenuPageWidget> PageClass);
	/** Shows again what showed before the top page: the page that opened it, or TopLevel. */
	void GoBack();
	/** Leaves every page for TopLevel; a menu that closes itself (the pause menu) closes instead. */
	UFUNCTION()
	virtual void CloseMenu();

protected:
	/** Starts on TopLevel with every page collapsed: the menu is reused on its next open. */
	virtual void NativeConstruct() override;
	/** Calls CloseMenu and consumes the Escape input, from any page. */
	virtual bool HandleEscapeAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> TopLevel;

private:
	/** Shows the last page of PageStack, or TopLevel when it is empty, and collapses every other page. */
	void ShowTop();
	/** Focuses what ShowTop shows. */
	void FocusTop();

	/** The pages opened, oldest first; the last one shows. A page opened twice is in it twice, so Back retraces the
	 * path rather than skipping a loop. */
	UPROPERTY()
	TArray<TObjectPtr<UGeoMenuPageWidget>> PageStack;
};
