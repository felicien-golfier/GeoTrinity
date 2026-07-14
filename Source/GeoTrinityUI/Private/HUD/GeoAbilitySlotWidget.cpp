// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoAbilitySlotWidget.h"

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	FName const CooldownFillParam(TEXT("Fill"));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::InitSlot(TArray<FGeoAbilityBarEntry> const& InEntries, AGeoHUD* InHUD)
{
	if (!ensureMsgf(InEntries.Num() > 0, TEXT("UGeoAbilitySlotWidget::InitSlot — no entries")))
	{
		return;
	}

	Entries = InEntries;
	DisplayedIndex = 0;
	HUD = InHUD;

	if (Icon && DisplayedEntry().Icon)
	{
		Icon->SetBrushFromTexture(const_cast<UTexture2D*>(DisplayedEntry().Icon.Get()));
	}

	if (CooldownSweep && CooldownSweepMaterial)
	{
		CooldownSweepMID = UMaterialInstanceDynamic::Create(CooldownSweepMaterial, this);
		CooldownSweepMID->SetScalarParameterValue(CooldownFillParam, 0.f);
		CooldownSweep->SetBrushFromMaterial(CooldownSweepMID);
	}

	if (CountdownText)
	{
		CountdownText->SetVisibility(ESlateVisibility::Hidden);
	}

	if (CountText)
	{
		CountText->SetVisibility(DisplayedEntry().bIsDeployable ? ESlateVisibility::HitTestInvisible
																: ESlateVisibility::Collapsed);
	}
	if (ChargesText)
	{
		ChargesText->SetVisibility(DisplayedEntry().bIsDeployable ? ESlateVisibility::HitTestInvisible
																  : ESlateVisibility::Collapsed);
	}

	RefreshDeployCount();
	RefreshDeployCharges();
	RefreshKeyLabel();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::SelectDisplayedEntry()
{
	// Last active-or-activatable entry wins so the armed follow-up (e.g. sacrifice detonate, gated by its
	// ActivationRequiredTags) overrides the base ability; the first entry is the resting look.
	int32 NewIndex = 0;
	for (int32 Index = Entries.Num() - 1; Index > 0; --Index)
	{
		FGameplayTag const& Tag = Entries[Index].AbilityTag;
		if (HUD->IsAbilityActive(Tag) || HUD->CanActivateAbility(Tag))
		{
			NewIndex = Index;
			break;
		}
	}

	if (NewIndex == DisplayedIndex)
	{
		return;
	}
	DisplayedIndex = NewIndex;

	if (Icon && DisplayedEntry().Icon)
	{
		Icon->SetBrushFromTexture(const_cast<UTexture2D*>(DisplayedEntry().Icon.Get()));
	}
	if (CountText)
	{
		CountText->SetVisibility(DisplayedEntry().bIsDeployable ? ESlateVisibility::HitTestInvisible
																: ESlateVisibility::Collapsed);
	}
	if (ChargesText)
	{
		ChargesText->SetVisibility(DisplayedEntry().bIsDeployable ? ESlateVisibility::HitTestInvisible
																  : ESlateVisibility::Collapsed);
	}
	RefreshDeployCount();
	RefreshDeployCharges();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::RefreshKeyLabel()
{
	if (!KeyText || !DisplayedEntry().InputAction)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	if (!InputSubsystem)
	{
		return;
	}

	TArray<FKey> const Keys = InputSubsystem->QueryKeysMappedToAction(DisplayedEntry().InputAction);
	FKey const Key = Keys.Num() > 0 ? Keys[0] : FKey();

	// Re-queried every tick; only touch the text when the binding actually changed so idle slots stay cheap.
	if (Key == CachedKey)
	{
		return;
	}
	CachedKey = Key;
	KeyText->SetText(Key.IsValid() ? Key.GetDisplayName(false) : FText::GetEmpty());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::RefreshDeployCount()
{
	if (!DisplayedEntry().bIsDeployable || !CountText || !HUD)
	{
		return;
	}

	int32 Current = 0;
	int32 Max = 0;
	HUD->GetDeployCountForAbility(DisplayedEntry().AbilityTag, Current, Max);
	CountText->SetText(FText::AsNumber(Max - Current));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::RefreshDeployCharges()
{
	if (!DisplayedEntry().bIsDeployable || !ChargesText || !HUD)
	{
		return;
	}

	int32 Current = 0;
	int32 Max = 0;
	HUD->GetDeployChargesForAbility(DisplayedEntry().AbilityTag, Current, Max);
	ChargesText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Current, Max)));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HUD || Entries.Num() == 0)
	{
		return;
	}

	SelectDisplayedEntry();
	RefreshKeyLabel();
	RefreshDeployCharges();

	FGameplayTag const& AbilityTag = DisplayedEntry().AbilityTag;
	float Remaining = 0.f;
	float Duration = 0.f;
	HUD->GetAbilityCooldown(AbilityTag, Remaining, Duration);

	// While the ability is active, keep the sweep fully filled so the slot reads as "in use". The cooldown (if any)
	// takes over depleting it once the ability ends; an active ability with no cooldown stays grayed until it ends.
	if (Remaining <= 0.f && HUD->IsAbilityActive(AbilityTag)
		&& !GeoASLib::GetAbilityCDO(AbilityTag)->GetClass()->IsChildOf(UGeoAutomaticFireAbility::StaticClass()))
	{
		if (CooldownSweepMID)
		{
			CooldownSweepMID->SetScalarParameterValue(CooldownFillParam, 1.f);
		}
		if (CountdownText && CountdownText->GetVisibility() != ESlateVisibility::Hidden)
		{
			CountdownText->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}

	// Ability is ready: clear the countdown text and the sweep fill. Both are normalized independently — the sweep
	// can be left pinned full by the active branch above (which hides the countdown without filling it), so gating the
	// sweep clear on the countdown's visibility would strand the fill at 1.0 until another ability drove a real
	// cooldown. The SetScalarParameterValue is a no-op when the value is unchanged, so idle slots stay cheap.
	if (Remaining <= 0.f)
	{
		if (CountdownText && CountdownText->GetVisibility() != ESlateVisibility::Hidden)
		{
			CountdownText->SetVisibility(ESlateVisibility::Hidden);
		}
		if (CooldownSweepMID)
		{
			CooldownSweepMID->SetScalarParameterValue(CooldownFillParam, 0.f);
		}
		return;
	}

	if (CooldownSweepMID && Duration > 0.f)
	{
		CooldownSweepMID->SetScalarParameterValue(CooldownFillParam, Remaining / Duration);
	}

	if (CountdownText)
	{
		CountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		CountdownText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Remaining)));
	}
}
