// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace
{
	UEnhancedInputUserSettings* GetUserSettings(ULocalPlayer* LocalPlayer)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		return InputSubsystem ? InputSubsystem->GetUserSettings() : nullptr;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::InitBinding(FName InMappingName, EPlayerMappableKeySlot InSlot, bool bInGamepad)
{
	MappingName = InMappingName;
	Slot = InSlot;
	bGamepad = bInGamepad;
	OnKeySelected.AddUniqueDynamic(this, &UGeoKeyBindingSelector::HandleBindingKeySelected);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::HandleBindingKeySelected(FInputChord SelectedKeyChord)
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings(GetOwningLocalPlayer());
	if (!ensureMsgf(UserSettings, TEXT("UGeoKeyBindingSelector: Enhanced Input user settings are not available")))
	{
		return;
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.Slot = Slot;
	Args.NewKey = SelectedKeyChord.Key;

	FGameplayTagContainer FailureReason;
	if (SelectedKeyChord.Key.IsGamepadKey() == bGamepad)
	{
		UserSettings->MapPlayerKey(Args, FailureReason);
	}
	if (SelectedKeyChord.Key.IsGamepadKey() != bGamepad || !FailureReason.IsEmpty())
	{
		FPlayerKeyMapping const* Current = UserSettings->FindCurrentMappingForSlot(MappingName, Slot);
		SetSelectedKey(FInputChord(Current ? Current->GetCurrentKey() : FKey()));
		UE_LOG(LogTemp, Log, TEXT("UGeoKeyBindingSelector: rejected key %s for %s (%s)"),
			   *SelectedKeyChord.Key.ToString(), *MappingName.ToString(), *FailureReason.ToString());
		return;
	}
	UserSettings->SaveSettings();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoSettingsWidget: BackButton is not bound"));
		return;
	}
	if (!MasterVolumeSlider)
	{
		ensureMsgf(MasterVolumeSlider, TEXT("UGeoSettingsWidget: MasterVolumeSlider is not bound"));
		return;
	}
	if (!KeyBindingsList)
	{
		ensureMsgf(KeyBindingsList, TEXT("UGeoSettingsWidget: KeyBindingsList is not bound"));
		return;
	}

	BackButton->OnClicked.AddDynamic(this, &UGeoSettingsWidget::HandleBack);
	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UGeoSettingsWidget::HandleMasterVolumeChanged);

	KeyBindingsList->ClearChildren();
	BuildKeyBindingsList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UGeoSettingsWidget::HandleBack);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.RemoveDynamic(this, &UGeoSettingsWidget::HandleMasterVolumeChanged);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::HandleMasterVolumeChanged(float /*Value*/)
{
	// Placeholder: no sound-class/mixer convention exists in the project yet.
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::BuildKeyBindingsList()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings(GetOwningLocalPlayer());
	UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	if (!ensureMsgf(Profile, TEXT("UGeoSettingsWidget: no active Enhanced Input key profile — is bEnableUserSettings "
								  "on and the mapping context registered?")))
	{
		return;
	}

	struct FBindingRow
	{
		FName MappingName;
		FText DisplayName;
		TArray<FPlayerKeyMapping const*> Keyboard;
		FPlayerKeyMapping const* Gamepad = nullptr;
	};

	TArray<FBindingRow> Rows;
	for (TPair<FName, FKeyMappingRow> const& RowPair : Profile->GetPlayerMappingRows())
	{
		FBindingRow Row;
		Row.MappingName = RowPair.Key;
		for (FPlayerKeyMapping const& Mapping : RowPair.Value.Mappings)
		{
			// 2D-stick mappings (move/aim sticks) cannot be captured by a key press and stay fixed.
			if (Mapping.GetDefaultKey().IsAxis2D())
			{
				continue;
			}
			Row.DisplayName = Mapping.GetDisplayName();
			if (Mapping.GetDefaultKey().IsGamepadKey())
			{
				Row.Gamepad = &Mapping;
			}
			else
			{
				Row.Keyboard.Add(&Mapping);
			}
		}
		if (Row.Keyboard.Num() > 0 || Row.Gamepad)
		{
			Rows.Add(Row);
		}
	}
	Rows.Sort(
		[](FBindingRow const& A, FBindingRow const& B)
		{
			return A.DisplayName.CompareTo(B.DisplayName) < 0;
		});

	for (FBindingRow const& Row : Rows)
	{
		// One UI line per keyboard mapping (WASD-style rows share one mapping name); the gamepad cell sits on the
		// first line only.
		int32 const LineCount = FMath::Max(Row.Keyboard.Num(), 1);
		for (int32 Line = 0; Line < LineCount; ++Line)
		{
			UHorizontalBox* RowBox = NewObject<UHorizontalBox>(WidgetTree);
			FPlayerKeyMapping const* Keyboard = Row.Keyboard.IsValidIndex(Line) ? Row.Keyboard[Line] : nullptr;

			UTextBlock* Label = NewObject<UTextBlock>(WidgetTree);
			Label->SetText(Row.Keyboard.Num() > 1
							   ? FText::Format(NSLOCTEXT("GeoSettings", "BindingRowFormat", "{0} ({1})"),
											   Row.DisplayName, Keyboard->GetDefaultKey().GetDisplayName(false))
							   : Row.DisplayName);
			UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(Label);
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetVerticalAlignment(VAlign_Center);

			AddBindingCell(RowBox, Row.MappingName, Keyboard, false);
			AddBindingCell(RowBox, Row.MappingName, Line == 0 ? Row.Gamepad : nullptr, true);

			KeyBindingsList->AddChild(RowBox);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSettingsWidget::AddBindingCell(UHorizontalBox* RowBox, FName MappingName, FPlayerKeyMapping const* Mapping,
										bool bGamepad)
{
	UWidget* Cell = nullptr;
	if (Mapping)
	{
		UGeoKeyBindingSelector* Selector = NewObject<UGeoKeyBindingSelector>(WidgetTree);
		Selector->InitBinding(MappingName, Mapping->GetSlot(), bGamepad);
		Selector->SetAllowGamepadKeys(bGamepad);
		Selector->SetAllowModifierKeys(false);
		Selector->SetEscapeKeys({EKeys::Escape});
		Selector->SetSelectedKey(FInputChord(Mapping->GetCurrentKey()));

		FTextBlockStyle KeyTextStyle = Selector->GetTextStyle();
		KeyTextStyle.Font.Size = 14;
		Selector->SetTextStyle(KeyTextStyle);
		Cell = Selector;
	}
	else
	{
		Cell = NewObject<USpacer>(this);
	}

	UHorizontalBoxSlot* CellSlot = RowBox->AddChildToHorizontalBox(Cell);
	CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	CellSlot->SetPadding(FMargin(8.f, 2.f));
	CellSlot->SetVerticalAlignment(VAlign_Center);
}
