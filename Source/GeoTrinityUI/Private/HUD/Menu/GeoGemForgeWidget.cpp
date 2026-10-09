// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoGemForgeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Menu/GeoTableWidget.h"
#include "HUD/Style/GeoGemGlyph.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BreakAllButton)
	{
		BreakAllButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleBreakAll);
	}
	if (BreakLessButton)
	{
		BreakLessButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleBreakLess);
	}
	if (BreakMoreButton)
	{
		BreakMoreButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleBreakMore);
	}
	if (BreakMaxButton)
	{
		BreakMaxButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleBreakMax);
	}
	if (BreakQuantityBox)
	{
		BreakQuantityBox->OnTextCommitted.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleBreakQuantityCommitted);
	}
	if (BreakButton)
	{
		BreakButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleBreak);
	}
	if (CraftLessButton)
	{
		CraftLessButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleCraftLess);
	}
	if (CraftMoreButton)
	{
		CraftMoreButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleCraftMore);
	}
	if (CraftMaxButton)
	{
		CraftMaxButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleCraftMax);
	}
	if (CraftQuantityBox)
	{
		CraftQuantityBox->OnTextCommitted.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleCraftQuantityCommitted);
	}
	if (CraftButton)
	{
		CraftButton->OnClicked.AddUniqueDynamic(this, &UGeoGemForgeWidget::HandleCraft);
	}
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::Refresh()
{
	UGeoGemProfileSave const* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!Profile || !Catalog)
	{
		UE_LOG(LogTemp, Log, TEXT("UGeoGemForgeWidget: no gem profile or catalog yet, forge left empty"));
		return;
	}

	FGeoGemList const* ShownList = Catalog->GemsByTier.Find(ShownTier);
	if (!Catalog->Find(PickedGem) || Catalog->FindTier(PickedGem) != ShownTier)
	{
		PickedGem = ShownList && !ShownList->Gems.IsEmpty() ? ShownList->Gems[0].Id : NAME_None;
	}
	ShowTierBreaks(*Profile, *Catalog);
	ShowRates(*Catalog);
	ShowGems(*Profile, *Catalog);
	ShowPicked(*Profile, *Catalog);
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoGemForgeWidget::GetBreakableCount(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog,
											 EGeoGemTier const Tier) const
{
	int32 Breakable = 0;
	FGeoGemList const* List = Catalog.GemsByTier.Find(Tier);
	for (FGeoGemInfo const& Gem : List ? List->Gems : TArray<FGeoGemInfo>())
	{
		Breakable += Profile.GetBreakableCount(Gem.Id);
	}
	return Breakable;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::ShowTierBreaks(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	int32 AllCount = 0;
	int32 AllShards = 0;
	if (TierBreakBox)
	{
		TierBreakBox->ClearChildren();
	}

	for (EGeoGemTier const Tier : GetTiers())
	{
		int32 Owned = 0;
		FGeoGemList const* List = Catalog.GemsByTier.Find(Tier);
		for (FGeoGemInfo const& Gem : List ? List->Gems : TArray<FGeoGemInfo>())
		{
			Owned += Profile.GetOwnedCount(Gem.Id);
		}
		int32 const Breakable = GetBreakableCount(Profile, Catalog, Tier);
		int32 const Shards = Breakable * Catalog.GetBreakDownShards(Tier);
		bool const bSelected = IsBreakSelected(Profile, Catalog, Tier);
		if (bSelected)
		{
			AllCount += Breakable;
			AllShards += Shards;
		}

		UGeoListRowWidget* Row =
			TierBreakBox ? MakeRow(bSelected ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
								   Breakable > 0 ? TFunction<void()>([this, Tier]
																	 {
																		 if (BreakTiers.Remove(Tier) == 0)
																		 {
																			 BreakTiers.Add(Tier);
																		 }
																		 Refresh();
																	 })
												 : TFunction<void()>())
						 : nullptr;
		if (Row)
		{
			UGeoGemGlyph* Glyph = WidgetTree->ConstructWidget<UGeoGemGlyph>(UGeoGemGlyph::StaticClass());
			Glyph->SetSize(TierGlyphSize);
			Glyph->SetSocket(Tier, false, 1);
			Row->AddColumn(Glyph, 0.f);
			UVerticalBox* Names = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Names->AddChildToVerticalBox(MakeText(EGeoTextRole::Button, GetTierName(Tier, true)));
			Names->AddChildToVerticalBox(
				MakeText(EGeoTextRole::Label, FText::Format(TierSubFormat, Owned, Owned - Breakable)));
			Row->AddColumn(Names, 1.f);
			Row->AddColumn(MakeText(EGeoTextRole::Mono, Breakable > 0
															? FText::Format(TierBreakFormat, Breakable, Shards)
															: NoneFreeText),
						   0.f);
			TierBreakBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
	}

	bool const bAnySelected = AllCount > 0;
	float const SummaryAlpha = bAnySelected ? 1.f : BreakIdleAlpha;
	if (BreakAllButton)
	{
		BreakAllButton->SetIsEnabled(bAnySelected);
	}
	if (BreakCountText)
	{
		BreakCountText->SetText(FText::Format(BreakCountFormat, AllCount));
		BreakCountText->SetColorAndOpacity(FSlateColor(BreakCountColor.GetColor(SummaryAlpha)));
	}
	if (BreakShardsText)
	{
		BreakShardsText->SetText(FText::Format(BreakShardsFormat, AllShards));
		BreakShardsText->SetColorAndOpacity(FSlateColor(BreakShardsColor.GetColor(SummaryAlpha)));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::ShowRates(UGeoGemCatalog const& Catalog)
{
	if (!RateTable)
	{
		return;
	}

	RateTable->ClearLines();
	for (EGeoGemTier const Tier : GetTiers())
	{
		RateTable->AddLine(
			{RateTable->MakeCellText(GetTierName(Tier, false)),
			 RateTable->MakeCellText(FText::Format(RateBreakFormat, Catalog.GetBreakDownShards(Tier))),
			 RateTable->MakeCellText(FText::AsNumber(Catalog.GetCraftCost(Tier)))});
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::ShowGems(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	if (TierTabBox)
	{
		TierTabBox->ClearChildren();
		for (EGeoGemTier const Tier : GetTiers())
		{
			UGeoListRowWidget* Tab = MakeRow(Tier == ShownTier ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
											 [this, Tier]
											 {
												 ShownTier = Tier;
												 BreakQuantity = 1;
												 CraftQuantity = 1;
												 Refresh();
											 },
											 TabRowClass);
			if (Tab)
			{
				Tab->AddTextColumn(GetTierName(Tier, true), 0.f);
				TierTabBox->AddChildToHorizontalBox(Tab)->SetPadding(FMargin(0.f, 0.f, TierTabGap, 0.f));
			}
		}
	}

	bool const bHadFocus = GemListBox->HasFocusedDescendants();
	UGeoListRowWidget* PickedRow = nullptr;
	GemListBox->ClearChildren();
	FGeoGemList const* List = Catalog.GemsByTier.Find(ShownTier);
	for (FGeoGemInfo const& Gem : List ? List->Gems : TArray<FGeoGemInfo>())
	{
		FName const GemId = Gem.Id;
		UGeoListRowWidget* Row = MakeRow(GemId == PickedGem ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
										 [this, GemId]
										 {
											 PickedGem = GemId;
											 BreakQuantity = 1;
											 CraftQuantity = 1;
											 Refresh();
										 });
		if (Row)
		{
			Row->AddColumn(MakeGemGlyph(Catalog, GemId, GemGlyphSize), 0.f);
			UVerticalBox* Names = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Names->AddChildToVerticalBox(MakeText(EGeoTextRole::Button, Gem.DisplayName.ToUpper()));
			UTextBlock* Effect = MakeText(EGeoTextRole::Body, Gem.GetEffectText());
			Effect->SetAutoWrapText(true);
			Names->AddChildToVerticalBox(Effect);
			Row->AddColumn(Names, 1.f);
			Row->AddColumn(MakeText(EGeoTextRole::Mono, FText::Format(OwnedCellFormat, Profile.GetOwnedCount(GemId))),
						   0.f);
			Row->AddColumn(
				MakeText(EGeoTextRole::Mono, FText::Format(FreeCellFormat, Profile.GetBreakableCount(GemId))), 0.f);
			GemListBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
			PickedRow = GemId == PickedGem ? Row : PickedRow;
		}
	}

	if (bHadFocus && PickedRow)
	{
		PickedRow->FocusRow();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::ShowPicked(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	FGeoGemInfo const* Gem = Catalog.Find(PickedGem);
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(PickedGem);
	if (!Gem || !Tier)
	{
		return;
	}

	int32 const Owned = Profile.GetOwnedCount(PickedGem);
	int32 const Breakable = Profile.GetBreakableCount(PickedGem);
	if (PickedGlyph)
	{
		PickedGlyph->SetGem(*Tier, Gem->Color.GetColor(1.f));
	}
	if (PickedTierText)
	{
		PickedTierText->SetText(
			FText::Format(TierFormat, GetTiers().IndexOfByKey(*Tier) + 1, GetTierName(*Tier, false)));
	}
	if (PickedNameText)
	{
		PickedNameText->SetText(Gem->DisplayName.ToUpper());
	}
	if (PickedEffectText)
	{
		PickedEffectText->SetText(Gem->GetEffectText());
	}
	if (PickedCountsText)
	{
		PickedCountsText->SetText(FText::Format(CountsFormat, Owned, Owned - Breakable, Breakable));
	}

	BreakQuantity = FMath::Clamp(BreakQuantity, 1, FMath::Max(Breakable, 1));
	if (BreakQuantityBox)
	{
		BreakQuantityBox->SetText(FText::FromString(FString::FromInt(Breakable > 0 ? BreakQuantity : 0)));
	}
	if (BreakButton)
	{
		BreakButton->SetIsEnabled(Breakable > 0);
		BreakButton->SetLabel(Breakable > 0 ? FText::Format(BreakFormat, BreakQuantity,
															BreakQuantity * Catalog.GetBreakDownShards(*Tier))
											: NoFreeCopyText);
	}

	CraftQuantity = FMath::Max(CraftQuantity, 1);
	int32 const Cost = CraftQuantity * Catalog.GetCraftCost(*Tier);
	bool const bCanCraft = Cost <= Profile.GetShards();
	if (CraftQuantityBox)
	{
		CraftQuantityBox->SetText(FText::FromString(FString::FromInt(CraftQuantity)));
	}
	if (CraftButton)
	{
		CraftButton->SetIsEnabled(bCanCraft);
		CraftButton->SetLabel(bCanCraft ? FText::Format(CraftFormat, CraftQuantity, Cost)
										: FText::Format(NeedShardsFormat, Cost));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoGemForgeWidget::IsBreakSelected(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog,
										 EGeoGemTier const Tier) const
{
	return BreakTiers.Contains(Tier) && GetBreakableCount(Profile, Catalog, Tier) > 0;
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoGemForgeWidget::GetMaxCraftQuantity() const
{
	UGeoGemProfileSave const* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	TOptional<EGeoGemTier> const Tier = Catalog ? Catalog->FindTier(PickedGem) : TOptional<EGeoGemTier>();
	int32 const Cost = Tier ? Catalog->GetCraftCost(*Tier) : 0;
	return Profile && Cost > 0 ? FMath::Max(1, Profile->GetShards() / Cost) : 1;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleBreakAll()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!Profile || !Catalog)
	{
		return;
	}

	TArray<EGeoGemTier> const Tiers = GetTiers().FilterByPredicate(
		[this, Profile, Catalog](EGeoGemTier const Tier)
		{
			return IsBreakSelected(*Profile, *Catalog, Tier);
		});
	int32 Count = 0;
	int32 Shards = 0;
	for (EGeoGemTier const Tier : Tiers)
	{
		Count += GetBreakableCount(*Profile, *Catalog, Tier);
		Shards += Profile->BreakDownTier(*Catalog, Tier);
	}
	if (MessageText && Shards > 0)
	{
		MessageText->SetText(
			FText::Format(BrokenFormat, Shards, Count, Tiers.Num() == 1 ? GetTierName(Tiers[0], true) : AllGemsName));
	}
	BreakTiers.Empty();
	CommitChanges();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleBreakLess()
{
	--BreakQuantity;
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleBreakMore()
{
	++BreakQuantity;
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleBreakMax()
{
	UGeoGemProfileSave const* Profile = GetProfile();
	BreakQuantity = Profile ? Profile->GetBreakableCount(PickedGem) : 1;
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleBreakQuantityCommitted(FText const& Text, ETextCommit::Type /*CommitMethod*/)
{
	BreakQuantity = FCString::Atoi(*Text.ToString());
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleBreak()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	FGeoGemInfo const* Gem = Catalog ? Catalog->Find(PickedGem) : nullptr;
	int32 const Shards = Profile && Gem ? Profile->BreakDown(*Catalog, PickedGem, BreakQuantity) : 0;
	if (Shards > 0)
	{
		if (MessageText)
		{
			MessageText->SetText(FText::Format(BrokenFormat, Shards, BreakQuantity, Gem->DisplayName.ToUpper()));
		}
		BreakQuantity = 1;
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleCraftLess()
{
	--CraftQuantity;
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleCraftMore()
{
	++CraftQuantity;
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleCraftMax()
{
	CraftQuantity = GetMaxCraftQuantity();
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleCraftQuantityCommitted(FText const& Text, ETextCommit::Type /*CommitMethod*/)
{
	CraftQuantity = FCString::Atoi(*Text.ToString());
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemForgeWidget::HandleCraft()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	FGeoGemInfo const* Gem = Catalog ? Catalog->Find(PickedGem) : nullptr;
	TOptional<EGeoGemTier> const Tier = Catalog ? Catalog->FindTier(PickedGem) : TOptional<EGeoGemTier>();
	int32 const Cost = Tier ? CraftQuantity * Catalog->GetCraftCost(*Tier) : 0;
	if (Profile && Gem && Profile->Craft(*Catalog, PickedGem, CraftQuantity))
	{
		if (MessageText)
		{
			MessageText->SetText(FText::Format(CraftedFormat, CraftQuantity, Gem->DisplayName.ToUpper(), Cost));
		}
		CraftQuantity = 1;
		CommitChanges();
	}
}
