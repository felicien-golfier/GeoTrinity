// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoButton.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoKeyBindingsWidget.generated.h"

class UGeoMenuButton;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
enum class EPlayerMappableKeySlot : uint8;
struct FPlayerKeyMapping;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoKeyBindingsClosedSignature);

/**
 * Key selector button for one player-mappable binding (one mapping name + slot, keyboard or gamepad). A UGeoButton
 * like every other menu button, so gamepad focus renders exactly like mouse-over. Clicking arms it (IsListening);
 * the owning UGeoKeyBindingsWidget feeds it the next pressed input via CommitKey, which writes the key to
 * UEnhancedInputUserSettings (rebuilding control mappings) and saves; keys of the wrong device type are rejected
 * and the display reverts to the current binding.
 */
UCLASS()
class GEOTRINITYUI_API UGeoKeyBindingSelector : public UGeoButton
{
	GENERATED_BODY()

public:
	void InitBinding(FName InMappingName, EPlayerMappableKeySlot InSlot, bool bInGamepad);

	bool IsListening() const
	{
		return bListening;
	}

	void CommitKey(FKey Key);
	void CancelListening();

private:
	UFUNCTION()
	void HandleClicked();

	void RefreshDisplayedKey();

	UPROPERTY()
	TObjectPtr<UTextBlock> KeyText;

	FName MappingName;
	EPlayerMappableKeySlot Slot;
	bool bGamepad = false;
	bool bListening = false;
};

/**
 * Key-bindings panel: KeyBindingsList is rebuilt from the Enhanced Input user settings' active key profile, one
 * row per player-mappable mapping with a keyboard and a gamepad UGeoKeyBindingSelector. Rows follow a fixed order:
 * movement first (forward, backward, left, right, dash), then attacks (basic, special, special alternatif),
 * reload and menu last. While a selector is listening, the panel captures the next key or mouse button in the
 * preview (tunnel) phase and commits it (Escape cancels). Communicates back to the parent menu exclusively via
 * the OnClosed delegate. Required in the BP hierarchy: UGeoMenuButton "BackButton", UScrollBox "KeyBindingsList".
 */
UCLASS()
class GEOTRINITYUI_API UGeoKeyBindingsWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Menu")
	FGeoKeyBindingsClosedSignature OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(FGeometry const& InGeometry, FKeyEvent const& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(FGeometry const& InGeometry,
												  FPointerEvent const& InMouseEvent) override;
	virtual UWidget* GetInitialFocusWidget() const override;
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UScrollBox> KeyBindingsList;

private:
	UFUNCTION()
	void HandleBack();

	void BuildKeyBindingsList();
	UGeoKeyBindingSelector* AddBindingCell(UHorizontalBox* RowBox, FName MappingName, FPlayerKeyMapping const* Mapping,
										   bool bGamepad);
	UGeoKeyBindingSelector* FindListeningSelector() const;

	UPROPERTY()
	TArray<TObjectPtr<UGeoKeyBindingSelector>> Selectors;
};
