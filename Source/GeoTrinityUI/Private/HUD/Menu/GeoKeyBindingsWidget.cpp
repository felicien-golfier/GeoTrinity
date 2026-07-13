// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoKeyBindingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
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

	// Row order of the key-bindings list; unknown mappings land after these, sorted by display name.
	FName const MappingOrder[] = {"MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "Dash",
								  "SpellBasic", "SpellSpecial", "SpellSpecialAlt", "Reload", "ToggleMenu"};
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::InitBinding(FName InMappingName, EPlayerMappableKeySlot InSlot, bool bInGamepad)
{
	MappingName = InMappingName;
	Slot = InSlot;
	bGamepad = bInGamepad;

	KeyText = NewObject<UTextBlock>(this);
	FSlateFontInfo KeyFont = KeyText->GetFont();
	KeyFont.Size = 14;
	KeyText->SetFont(KeyFont);
	KeyText->SetJustification(ETextJustify::Center);
	KeyText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	AddChild(KeyText);

	OnClicked.AddUniqueDynamic(this, &UGeoKeyBindingSelector::HandleClicked);
	RefreshDisplayedKey();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::HandleClicked()
{
	bListening = true;
	KeyText->SetText(NSLOCTEXT("GeoSettings", "KeyBindingListening", "..."));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::CommitKey(FKey Key)
{
	bListening = false;
	UEnhancedInputUserSettings* UserSettings = GetUserSettings(GetOwningLocalPlayer());
	if (!ensureMsgf(UserSettings, TEXT("UGeoKeyBindingSelector: Enhanced Input user settings are not available")))
	{
		return;
	}

	FGameplayTagContainer FailureReason;
	if (Key.IsGamepadKey() == bGamepad)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = MappingName;
		Args.Slot = Slot;
		Args.NewKey = Key;
		UserSettings->MapPlayerKey(Args, FailureReason);
	}
	if (Key.IsGamepadKey() != bGamepad || !FailureReason.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("UGeoKeyBindingSelector: rejected key %s for %s (%s)"), *Key.ToString(),
			   *MappingName.ToString(), *FailureReason.ToString());
	}
	else
	{
		UserSettings->SaveSettings();
	}
	RefreshDisplayedKey();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::CancelListening()
{
	bListening = false;
	RefreshDisplayedKey();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingSelector::RefreshDisplayedKey()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings(GetOwningLocalPlayer());
	FPlayerKeyMapping const* Current =
		UserSettings ? UserSettings->FindCurrentMappingForSlot(MappingName, Slot) : nullptr;
	KeyText->SetText(Current ? Current->GetCurrentKey().GetDisplayName(false) : FText::GetEmpty());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoKeyBindingsWidget: BackButton is not bound"));
		return;
	}
	if (!KeyBindingsList)
	{
		ensureMsgf(KeyBindingsList, TEXT("UGeoKeyBindingsWidget: KeyBindingsList is not bound"));
		return;
	}

	BackButton->OnClicked.AddDynamic(this, &UGeoKeyBindingsWidget::HandleBack);

	KeyBindingsList->ClearChildren();
	Selectors.Empty();
	BuildKeyBindingsList();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingsWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UGeoKeyBindingsWidget::HandleBack);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoKeyBindingsWidget::NativeOnPreviewKeyDown(FGeometry const& InGeometry, FKeyEvent const& InKeyEvent)
{
	if (UGeoKeyBindingSelector* Listening = FindListeningSelector())
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			Listening->CancelListening();
		}
		else
		{
			Listening->CommitKey(InKeyEvent.GetKey());
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoKeyBindingsWidget::NativeOnPreviewMouseButtonDown(FGeometry const& InGeometry,
															 FPointerEvent const& InMouseEvent)
{
	if (UGeoKeyBindingSelector* Listening = FindListeningSelector())
	{
		Listening->CommitKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoKeyBindingsWidget::GetInitialFocusWidget() const
{
	return BackButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoKeyBindingsWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingsWidget::HandleBack()
{
	OnClosed.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoKeyBindingSelector* UGeoKeyBindingsWidget::FindListeningSelector() const
{
	TObjectPtr<UGeoKeyBindingSelector> const* Listening =
		Selectors.FindByPredicate([](UGeoKeyBindingSelector const* Selector) { return Selector->IsListening(); });
	return Listening ? *Listening : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoKeyBindingsWidget::BuildKeyBindingsList()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings(GetOwningLocalPlayer());
	UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	if (!ensureMsgf(Profile, TEXT("UGeoKeyBindingsWidget: no active Enhanced Input key profile — is bEnableUserSettings "
								  "on and the mapping context registered?")))
	{
		return;
	}

	struct FBindingRow
	{
		FName MappingName;
		FText DisplayName;
		int32 OrderIndex = 0;
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
		if (Row.Keyboard.Num() == 0 && !Row.Gamepad)
		{
			continue;
		}
		Row.OrderIndex = TConstArrayView<FName>(MappingOrder).IndexOfByKey(Row.MappingName);
		if (Row.OrderIndex == INDEX_NONE)
		{
			Row.OrderIndex = UE_ARRAY_COUNT(MappingOrder);
			UE_LOG(LogTemp, Log, TEXT("UGeoKeyBindingsWidget: mapping %s has no defined row order, listing it last"),
				   *Row.MappingName.ToString());
		}
		Rows.Add(Row);
	}
	Rows.Sort(
		[](FBindingRow const& A, FBindingRow const& B)
		{
			return A.OrderIndex != B.OrderIndex ? A.OrderIndex < B.OrderIndex
												: A.DisplayName.CompareTo(B.DisplayName) < 0;
		});

	TArray<UGeoKeyBindingSelector*> GamepadColumn;
	for (FBindingRow const& Row : Rows)
	{
		// One UI line per keyboard mapping (rows sharing one mapping name); the gamepad cell sits on the
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
			FSlateChildSize LabelSize(ESlateSizeRule::Fill);
			LabelSize.Value = 0.7f;
			LabelSlot->SetSize(LabelSize);
			LabelSlot->SetVerticalAlignment(VAlign_Center);

			AddBindingCell(RowBox, Row.MappingName, Keyboard, false);
			if (UGeoKeyBindingSelector* GamepadSelector =
					AddBindingCell(RowBox, Row.MappingName, Line == 0 ? Row.Gamepad : nullptr, true))
			{
				GamepadColumn.Add(GamepadSelector);
			}

			KeyBindingsList->AddChild(RowBox);
		}
	}

	// Spacer lines (keyboard-only extra lines) let geometric vertical navigation escape the gamepad column,
	// so chain it explicitly.
	for (int32 Index = 1; Index < GamepadColumn.Num(); ++Index)
	{
		GamepadColumn[Index - 1]->SetNavigationRuleExplicit(EUINavigation::Down, GamepadColumn[Index]);
		GamepadColumn[Index]->SetNavigationRuleExplicit(EUINavigation::Up, GamepadColumn[Index - 1]);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoKeyBindingSelector* UGeoKeyBindingsWidget::AddBindingCell(UHorizontalBox* RowBox, FName MappingName,
															  FPlayerKeyMapping const* Mapping, bool bGamepad)
{
	UGeoKeyBindingSelector* Selector = nullptr;
	UWidget* Cell = nullptr;
	if (Mapping)
	{
		Selector = NewObject<UGeoKeyBindingSelector>(WidgetTree);
		Selector->InitBinding(MappingName, Mapping->GetSlot(), bGamepad);
		if (UGeoButton* StyleSource = BackButton->GetButtonWidget())
		{
			Selector->SetStyle(StyleSource->GetStyle());
		}
		Selectors.Add(Selector);
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
	return Selector;
}
