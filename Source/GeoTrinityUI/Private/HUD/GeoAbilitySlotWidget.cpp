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
void UGeoAbilitySlotWidget::InitSlot(FGeoAbilityBarEntry const& InEntry, AGeoHUD* InHUD)
{
	Entry = InEntry;
	HUD = InHUD;

	if (Icon && Entry.Icon)
	{
		Icon->SetBrushFromTexture(const_cast<UTexture2D*>(Entry.Icon.Get()));
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
		CountText->SetVisibility(Entry.bIsDeployable ? ESlateVisibility::HitTestInvisible
													 : ESlateVisibility::Collapsed);
	}

	RefreshDeployCount();
	RefreshKeyLabel();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::RefreshKeyLabel()
{
	if (!KeyText || !Entry.InputAction)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	if (!InputSubsystem)
	{
		return;
	}

	TArray<FKey> const Keys = InputSubsystem->QueryKeysMappedToAction(Entry.InputAction);
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
	if (!Entry.bIsDeployable || !CountText || !HUD)
	{
		return;
	}

	int32 Current = 0;
	int32 Max = 0;
	HUD->GetDeployCountForAbility(Entry.AbilityTag, Current, Max);
	CountText->SetText(FText::AsNumber(Current));
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilitySlotWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HUD)
	{
		return;
	}

	RefreshKeyLabel();

	float Remaining = 0.f;
	float Duration = 0.f;
	HUD->GetAbilityCooldown(Entry.AbilityTag, Remaining, Duration);

	// While the ability is active, keep the sweep fully filled so the slot reads as "in use". The cooldown (if any)
	// takes over depleting it once the ability ends; an active ability with no cooldown stays grayed until it ends.
	if (Remaining <= 0.f && HUD->IsAbilityActive(Entry.AbilityTag)
		&& !GeoASLib::GetAbilityCDO(Entry.AbilityTag)->GetClass()->IsChildOf(UGeoAutomaticFireAbility::StaticClass()))
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

	// Idle slots cost nothing: nothing to animate once the ability is ready and the sweep is already cleared.
	if (Remaining <= 0.f)
	{
		if (CountdownText && CountdownText->GetVisibility() != ESlateVisibility::Hidden)
		{
			CountdownText->SetVisibility(ESlateVisibility::Hidden);
			if (CooldownSweepMID)
			{
				CooldownSweepMID->SetScalarParameterValue(CooldownFillParam, 0.f);
			}
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
