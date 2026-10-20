// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/Frame/GeoMenuPageWidget.h"

#include "GeoTutorialPromptWidget.generated.h"

class UGeoMenuButton;

/**
 * First-launch tutorial prompt, a page of the main menu opened by it while bTutorialPromptAnswered is false. The Cat's
 * text is authored in the BP (Tutorial Board, page "Prompt"). Either answer sets the flag for good; do it opens the
 * tutorial map, skip goes back to the menu.
 * Required in the BP hierarchy: UGeoMenuButton "DoItButton", "SkipButton"; worn frame: the bare UGeoMenuFrameWidget
 * (no Back, no close cross: the two buttons are the only ways out).
 */
UCLASS()
class GEOTRINITYUI_API UGeoTutorialPromptWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

protected:
	/** Wires both buttons. */
	virtual void NativeConstruct() override;
	/** Returns DoItButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> DoItButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SkipButton;

private:
	UFUNCTION()
	void HandleDoIt();

	UFUNCTION()
	void HandleSkip();
};
