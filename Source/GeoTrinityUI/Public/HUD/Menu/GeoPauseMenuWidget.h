// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuRootWidget.h"

#include "GeoPauseMenuWidget.generated.h"

class UGeoMenuButton;

/**
 * In-game pause menu opened via AGeoPlayerController's toggle-menu input. Owned and shown/hidden by the
 * PlayerController, not nested in a parent menu; closing it, from any page, resumes the game.
 * Required in the BP hierarchy: UGeoMenuButton "ResumeButton", "SettingsButton", "LeaderboardButton",
 * "ReturnToMainMenuButton", "QuitButton", and as pages a UGeoSettingsWidget (with the UGeoSoundSettingsWidget,
 * UGeoKeyBindingsWidget and UGeoInterfaceSettingsWidget it opens) and a UGeoLeaderboardWidget. Optional:
 * "CharacterButton" with a
 * UGeoCharacterSheetWidget page, and the UGeoGemsWidget page that sheet opens.
 * The leaderboard is the same page class the main menu opens, and it reads the save on every construct, so a run
 * finished this session shows up without leaving the level.
 */
UCLASS()
class GEOTRINITYUI_API UGeoPauseMenuWidget : public UGeoMenuRootWidget
{
	GENERATED_BODY()

public:
	/** Closes the pause menu. */
	virtual void CloseMenu() override;

protected:
	/** Wires each pause-menu button. */
	virtual void NativeConstruct() override;
	/** Returns ResumeButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Closes the pause menu and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> LeaderboardButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> ReturnToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> CharacterButton;

private:
	UFUNCTION()
	void HandleCharacter();

	UFUNCTION()
	void HandleSettings();

	UFUNCTION()
	void HandleLeaderboard();

	UFUNCTION()
	void HandleReturnToMainMenu();

	UFUNCTION()
	void HandleQuit();
};
