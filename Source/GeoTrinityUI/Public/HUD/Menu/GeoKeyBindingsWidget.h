// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/InputKeySelector.h"
#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoKeyBindingsWidget.generated.h"

class UGeoMenuButton;
class UHorizontalBox;
class UScrollBox;
enum class EPlayerMappableKeySlot : uint8;
struct FPlayerKeyMapping;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoKeyBindingsClosedSignature);

/**
 * Key selector for one player-mappable binding (one mapping name + slot, keyboard or gamepad). On selection it
 * writes the new key to UEnhancedInputUserSettings (which rebuilds control mappings) and saves; keys of the wrong
 * device type are rejected and the display reverts to the current binding.
 */
UCLASS()
class GEOTRINITYUI_API UGeoKeyBindingSelector : public UInputKeySelector
{
	GENERATED_BODY()

public:
	void InitBinding(FName InMappingName, EPlayerMappableKeySlot InSlot, bool bInGamepad);

private:
	UFUNCTION()
	void HandleBindingKeySelected(FInputChord SelectedKeyChord);

	FName MappingName;
	EPlayerMappableKeySlot Slot;
	bool bGamepad = false;
};

/**
 * Key-bindings panel: KeyBindingsList is rebuilt from the Enhanced Input user settings' active key profile, one
 * row per player-mappable mapping with a keyboard and a gamepad UGeoKeyBindingSelector. Rows follow a fixed order:
 * movement first (forward, backward, left, right, dash), then attacks (basic, special, special alternatif),
 * reload and menu last. Communicates back to the parent menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "BackButton", UScrollBox "KeyBindingsList".
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
	void AddBindingCell(UHorizontalBox* RowBox, FName MappingName, FPlayerKeyMapping const* Mapping, bool bGamepad);
};
