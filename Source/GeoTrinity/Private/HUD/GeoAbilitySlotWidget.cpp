// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoAbilitySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
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

	float Remaining = 0.f;
	float Duration = 0.f;
	HUD->GetAbilityCooldown(Entry.AbilityTag, Remaining, Duration);

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
			tmp = MAX_FLT;
		}
		return;
	}

	if (tmp < Remaining)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cooldown sweep animation is not in sync with the ability cooldown!"));
	}

	tmp = Remaining;
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
