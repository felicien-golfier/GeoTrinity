// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoGemPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemSubsystem.h"
#include "HUD/Style/GeoGemGlyph.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
TArray<EGeoGemTier> const& UGeoGemPageWidget::GetTiers()
{
	static TArray<EGeoGemTier> const Tiers = {EGeoGemTier::Chip, EGeoGemTier::Cut, EGeoGemTier::Prism,
											  EGeoGemTier::Core};
	return Tiers;
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoGemProfileSave* UGeoGemPageWidget::GetProfile() const
{
	UGeoGemSubsystem const* GemSubsystem = ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(GetOwningLocalPlayer());
	return GemSubsystem ? GemSubsystem->GetProfile() : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemPageWidget::CommitChanges()
{
	CommitProfile();
	Refresh();
	OnProfileChanged.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemPageWidget::CommitProfile()
{
	if (UGeoGemSubsystem* GemSubsystem = ULocalPlayer::GetSubsystem<UGeoGemSubsystem>(GetOwningLocalPlayer()))
	{
		GemSubsystem->CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoListRowWidget* UGeoGemPageWidget::MakeRow(EGeoListRowTint const Tint, TFunction<void()> Picked,
											   TSubclassOf<UGeoListRowWidget> Class)
{
	Class = Class ? Class : RowClass;
	if (!ensureMsgf(Class, TEXT("%hs: no row class on %s"), __FUNCTION__, *GetName()))
	{
		return nullptr;
	}

	UGeoListRowWidget* Row = CreateWidget<UGeoListRowWidget>(this, Class);
	Row->SetTint(Tint);
	Row->SetSelectable(Picked != nullptr);
	if (Picked)
	{
		Row->OnClicked.AddWeakLambda(this, MoveTemp(Picked));
	}
	return Row;
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoGemPageWidget::MakeText(EGeoTextRole const Role, FText const& Text) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UGeoUITheme::ApplyTextStyle(TextBlock, Role);
	TextBlock->SetText(Text);
	return TextBlock;
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoGemGlyph* UGeoGemPageWidget::MakeGemGlyph(UGeoGemCatalog const& Catalog, FName const GemId,
											   float const GlyphSize) const
{
	UGeoGemGlyph* Glyph = WidgetTree->ConstructWidget<UGeoGemGlyph>(UGeoGemGlyph::StaticClass());
	Glyph->SetSize(GlyphSize);
	FGeoGemInfo const* Gem = Catalog.Find(GemId);
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(GemId);
	if (ensureMsgf(Gem && Tier, TEXT("%hs: no gem %s in %s"), __FUNCTION__, *GemId.ToString(), *Catalog.GetName()))
	{
		Glyph->SetGem(*Tier, Gem->Color.GetColor(1.f));
	}
	return Glyph;
}

// ---------------------------------------------------------------------------------------------------------------------
FText UGeoGemPageWidget::GetTierName(EGeoGemTier const Tier, bool const bPlural) const
{
	return (bPlural ? TierPluralNames : TierNames).FindRef(Tier);
}
