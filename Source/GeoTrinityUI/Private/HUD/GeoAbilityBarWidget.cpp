// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoAbilityBarWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "HUD/GeoAbilitySlotWidget.h"

namespace
{
	/** Small horizontal gap (px each side) between packed ability slots. */
	constexpr float SlotGap = 3.f;
} // namespace

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityBarWidget::BuildBar(AGeoHUD* InHUD, APlayableCharacter* PlayableCharacter)
{
	if (!ensureMsgf(InHUD, TEXT("UGeoAbilityBarWidget::BuildBar — HUD is null"))
		|| !ensureMsgf(SlotBox, TEXT("UGeoAbilityBarWidget::BuildBar — SlotBox is not bound"))
		|| !ensureMsgf(SlotWidgetClass, TEXT("UGeoAbilityBarWidget::BuildBar — SlotWidgetClass is not set")))
	{
		return;
	}

	HUD = InHUD;

	SlotBox->ClearChildren();
	Slots.Reset();

	// Abilities sharing one input (sacrifice channel/detonate) collapse into a single slot that swaps between them.
	TArray<TArray<FGeoAbilityBarEntry>> GroupedEntries;
	for (FGeoAbilityBarEntry const& Entry : HUD->GetAbilityBarEntries(PlayableCharacter))
	{
		TArray<FGeoAbilityBarEntry>* Group = GroupedEntries.FindByPredicate(
			[&Entry](TArray<FGeoAbilityBarEntry> const& Candidate)
			{
				return Candidate[0].InputTag == Entry.InputTag;
			});
		if (Group)
		{
			Group->Add(Entry);
		}
		else
		{
			GroupedEntries.Add({Entry});
		}
	}

	for (TArray<FGeoAbilityBarEntry> const& Group : GroupedEntries)
	{
		UGeoAbilitySlotWidget* SlotWidget = CreateWidget<UGeoAbilitySlotWidget>(this, SlotWidgetClass);
		SlotWidget->InitSlot(Group, HUD);

		// Auto-size so each slot is only as wide as its square (whose side = the bar height), with a small gap between
		// slots. The SlotBox is centered in the bar (see BuildAbilityBarWidget), so the packed run of squares stays
		// centered and resolution-independent (the bar is a fixed fraction of the screen).
		if (UHorizontalBoxSlot* BoxSlot = SlotBox->AddChildToHorizontalBox(SlotWidget))
		{
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			BoxSlot->SetPadding(FMargin(SlotGap, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Fill);
		}
		Slots.Add(SlotWidget);
	}

	// AddUnique so repeated BuildBar calls (class change) leave exactly one binding.
	HUD->OnPlayerDeployCountChanged.AddUniqueDynamic(this, &UGeoAbilityBarWidget::OnDeployCountChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityBarWidget::OnDeployCountChanged()
{
	for (UGeoAbilitySlotWidget* SlotWidget : Slots)
	{
		SlotWidget->RefreshDeployCount();
	}
}
