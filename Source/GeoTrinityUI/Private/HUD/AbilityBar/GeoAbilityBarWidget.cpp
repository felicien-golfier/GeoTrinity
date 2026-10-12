// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/AbilityBar/GeoAbilityBarWidget.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Components/HorizontalBox.h"
#include "Algo/Transform.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "HUD/AbilityBar/GeoAbilitySlotWidget.h"

namespace
{
	/** Small horizontal gap (px each side) between packed ability slots. */
	constexpr float SlotGap = 3.f;

	/** Extra gap left of the dash slot, setting movement apart from the ability run — half of WBP_AbilitySlot's
	 * 64px square. */
	constexpr float DashSlotGap = 32.f;
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
	Character = PlayableCharacter;

	SlotBox->ClearChildren();
	Slots.Reset();
	if (PassiveBox)
	{
		PassiveBox->ClearChildren();
	}
	ShownPassiveTags.Reset();

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
			bool const bIsDash = Group[0].InputTag == FGeoGameplayTags::Get().InputTag_Dash;
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			BoxSlot->SetPadding(FMargin(bIsDash ? SlotGap + DashSlotGap : SlotGap, 0.f, SlotGap, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Fill);
		}
		Slots.Add(SlotWidget);
	}

	// AddUnique so repeated BuildBar calls (class change) leave exactly one binding.
	HUD->OnPlayerDeployCountChanged.AddUniqueDynamic(this, &UGeoAbilityBarWidget::OnDeployCountChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityBarWidget::NativeTick(FGeometry const& MyGeometry, float const InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PassiveBox && HUD && Character)
	{
		TArray<FGeoAbilityBarEntry> const Passives = HUD->GetPassiveBarEntries(Character);
		TArray<FGameplayTag> PassiveTags;
		Algo::Transform(Passives, PassiveTags, &FGeoAbilityBarEntry::AbilityTag);
		if (PassiveTags != ShownPassiveTags)
		{
			ShownPassiveTags = PassiveTags;
			BuildPassiveTiles(Passives);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityBarWidget::BuildPassiveTiles(TArray<FGeoAbilityBarEntry> const& Passives)
{
	PassiveBox->ClearChildren();
	if (ensureMsgf(PassiveSlotWidgetClass, TEXT("%hs: PassiveSlotWidgetClass is not set"), __FUNCTION__))
	{
		for (FGeoAbilityBarEntry const& Passive : Passives)
		{
			UGeoAbilitySlotWidget* Tile = CreateWidget<UGeoAbilitySlotWidget>(this, PassiveSlotWidgetClass);
			Tile->InitSlot({Passive}, HUD);
			PassiveBox->AddChildToWrapBox(Tile)->SetPadding(FMargin(SlotGap, 0.f, SlotGap, 0.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoAbilityBarWidget::OnDeployCountChanged()
{
	for (UGeoAbilitySlotWidget* SlotWidget : Slots)
	{
		SlotWidget->RefreshDeployCount();
	}
}
