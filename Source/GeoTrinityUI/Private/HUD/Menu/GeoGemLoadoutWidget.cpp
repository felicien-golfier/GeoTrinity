// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoGemLoadoutWidget.h"

#include "Algo/AnyOf.h"
#include "Algo/Count.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameClasses/GeoPlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "HUD/Menu/GeoGemSocketButton.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "HUD/Style/GeoFrame.h"
#include "HUD/Style/GeoGemGlyph.h"
#include "HUD/Style/GeoMeter.h"
#include "HUD/Style/GeoShape.h"
#include "HUD/Style/GeoThemedInputs.h"
#include "HUD/Style/GeoUITheme.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::SetShown(UWidget* Widget, bool const bShown)
{
	if (Widget)
	{
		Widget->SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EquipButton)
	{
		EquipButton->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleEquip);
	}
	if (FillButton)
	{
		FillButton->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleFill);
	}
	if (UnequipButton)
	{
		UnequipButton->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleUnequip);
	}
	SetVisibility(ESlateVisibility::Visible);
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoGemLoadoutWidget::NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	FVector2D const Click = InMouseEvent.GetScreenSpacePosition();
	bool const bOnPickingPart = Algo::AnyOf(TArray<UWidget const*>{StackGrid, DetailBox},
											[&Click](UWidget const* Part)
											{
												return Part && Part->IsVisible()
													&& Part->GetCachedGeometry().IsUnderLocation(Click);
											});
	bool const bDrop = !bOnPickingPart && (!PickedGem.IsNone() || PickedSocket != INDEX_NONE);
	if (bDrop)
	{
		PickedGem = NAME_None;
		PickedSocket = INDEX_NONE;
		Refresh();
	}
	return bDrop ? FReply::Handled() : Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::Refresh()
{
	UGeoGemProfileSave const* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!Profile || !Catalog)
	{
		UE_LOG(LogGeoTrinity, Log, TEXT("UGeoGemLoadoutWidget: no gem profile or catalog yet, loadout left empty"));
		return;
	}

	AGeoPlayerState const* PlayerState = GetOwningPlayerState<AGeoPlayerState>();
	if (ShownClass == EPlayerClass::None && PlayerState)
	{
		ShownClass = PlayerState->GetPlayerClass();
	}
	if (SocketButtons.IsEmpty())
	{
		BuildBoard();
	}
	ShowClassTabs(*Profile);
	ShowInPlay(*Profile);
	ShowBuildTabs(*Profile);
	ShowLevel(*Profile);
	ShowSockets(*Profile, *Catalog);
	ShowFilters(*Profile, *Catalog);
	ShowStacks(*Profile, *Catalog);
	ShowDetail(*Profile, *Catalog);
	ShowTotals(*Profile, *Catalog);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::BuildBoard()
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	if (!ensureMsgf(Clusters.Num() > 0 && Sockets.Num() % Clusters.Num() == 0,
					TEXT("%hs: %d sockets do not share out between %d clusters on %s"), __FUNCTION__, Sockets.Num(),
					Clusters.Num(), *GetName()))
	{
		return;
	}

	int32 const SocketsPerCluster = Sockets.Num() / Clusters.Num();
	TMap<EGeoGemTier, int32> RingSocketCounts;
	float OuterRadius = 0.f;
	for (int32 Index = 0; Index < SocketsPerCluster; ++Index)
	{
		++RingSocketCounts.FindOrAdd(Sockets[Index].Tier);
		FGeoGemRing const Ring = Rings.FindRef(Sockets[Index].Tier);
		OuterRadius = FMath::Max(OuterRadius, Ring.Radius + .5f * Ring.SocketSize);
	}

	auto AddToBoard = [this](UWidget* Widget, FVector2D const& Position, FVector2D const& Size)
	{
		UCanvasPanelSlot* const BoardSlot = Board->AddChildToCanvas(Widget);
		BoardSlot->SetAutoSize(false);
		BoardSlot->SetSize(Size);
		BoardSlot->SetPosition(Position - .5f * Size);
	};
	auto AddCircle = [this, &AddToBoard](FVector2D const& Center, float const Radius, bool const bFilled,
										 FLinearColor const& Color)
	{
		UGeoShape* Circle = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
		Circle->Sides = 0;
		Circle->Size = 2.f * Radius;
		Circle->bFilled = bFilled;
		Circle->LineThickness = RingThickness;
		Circle->Color = Color;
		AddToBoard(Circle, Center, FVector2D(2.f * Radius));
	};

	Board->ClearChildren();
	SocketButtons.Reset();
	for (int32 ClusterIndex = 0; ClusterIndex < Clusters.Num(); ++ClusterIndex)
	{
		FGeoGemCluster const& Cluster = Clusters[ClusterIndex];
		AddCircle(Cluster.Center, OuterRadius + ClusterDiscMargin, true, ClusterDiscColor);
		for (TPair<EGeoGemTier, FGeoGemRing> const& Ring : Rings)
		{
			if (Ring.Value.Radius > 0.f)
			{
				AddCircle(Cluster.Center, Ring.Value.Radius, false, RingColor);
			}
		}

		UTextBlock* Label = MakeText(
			EGeoTextRole::Label,
			FText::Format(ClusterFormat, Cluster.Name, Sockets[ClusterIndex * SocketsPerCluster].UnlockLevel));
		UCanvasPanelSlot* const LabelSlot = Board->AddChildToCanvas(Label);
		LabelSlot->SetAutoSize(true);
		LabelSlot->SetPosition(Cluster.LabelPosition);
		LabelSlot->SetAlignment(Cluster.LabelAlignment);

		TMap<EGeoGemTier, int32> PlacedOnRing;
		for (int32 Index = ClusterIndex * SocketsPerCluster; Index < (ClusterIndex + 1) * SocketsPerCluster; ++Index)
		{
			EGeoGemTier const Tier = Sockets[Index].Tier;
			FGeoGemRing const Ring = Rings.FindRef(Tier);
			int32& Placed = PlacedOnRing.FindOrAdd(Tier);
			float const Angle =
				FMath::DegreesToRadians(Ring.StartAngle + Placed * 360.f / RingSocketCounts.FindRef(Tier));
			++Placed;

			UGeoGemGlyph* Glyph = WidgetTree->ConstructWidget<UGeoGemGlyph>(UGeoGemGlyph::StaticClass());
			Glyph->SetSize(Ring.SocketSize);
			UGeoGemSocketButton* Socket =
				WidgetTree->ConstructWidget<UGeoGemSocketButton>(UGeoGemSocketButton::StaticClass());
			Socket->InitSocket(Index, Glyph);
			Socket->OnPicked.BindUObject(this, &UGeoGemLoadoutWidget::PickSocket);
			AddToBoard(Socket, Cluster.Center + Ring.Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)),
					   FVector2D(Ring.SocketSize));
			SocketButtons.Add(Socket);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowClassTabs(UGeoGemProfileSave const& Profile)
{
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	if (!ClassTabBox || !Theme)
	{
		return;
	}

	ClassTabBox->ClearChildren();
	for (TPair<EPlayerClass, FGeoClassStyle> const& Class : Theme->ClassStyles)
	{
		EPlayerClass const PlayerClass = Class.Key;
		UGeoListRowWidget* Tab = MakeRow(PlayerClass == ShownClass ? EGeoListRowTint::Selected
																	 : EGeoListRowTint::Normal,
										 [this, PlayerClass]
										 {
											 ShownClass = PlayerClass;
											 PickedGem = NAME_None;
											 PickedSocket = INDEX_NONE;
											 bRenamingBuild = false;
											 Refresh();
										 });
		if (Tab)
		{
			UGeoShape* Shape = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
			Shape->Size = ClassTabShapeSize;
			Shape->SetClassShape(Class.Value);
			Tab->AddColumn(Shape, 0.f);
			Tab->AddTextColumn(Class.Value.Name, 0.f);
			Tab->AddColumn(
				MakeText(EGeoTextRole::Mono, FText::Format(ClassLevelFormat, Profile.GetClassLevel(PlayerClass))), 0.f);
			ClassTabBox->AddChildToHorizontalBox(Tab)->SetPadding(FMargin(0.f, 0.f, ClassTabGap, 0.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowInPlay(UGeoGemProfileSave const& Profile)
{
	TArray<FGeoGemBuild> const Builds = Profile.GetBuilds(ShownClass);
	int32 const ActiveBuild = Profile.GetActiveBuild(ShownClass);
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
	if (InPlayShape && ClassStyle)
	{
		InPlayShape->SetClassShape(*ClassStyle);
	}
	if (InPlayText)
	{
		InPlayText->SetText(FText::Format(InPlayFormat, GetBuildName(Builds[ActiveBuild], ActiveBuild)));
	}
	if (BuildCountText)
	{
		BuildCountText->SetText(FText::Format(BuildCountFormat, Builds.Num(), GeoGem::MaxBuilds));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowBuildTabs(UGeoGemProfileSave const& Profile)
{
	if (!BuildTabBox)
	{
		return;
	}

	TArray<FGeoGemBuild> const Builds = Profile.GetBuilds(ShownClass);
	int32 const ActiveBuild = Profile.GetActiveBuild(ShownClass);
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
	BuildTabBox->ClearChildren();
	auto AddTab = [this](UWidget* Tab)
	{
		if (Tab)
		{
			UHorizontalBoxSlot* const TabSlot = BuildTabBox->AddChildToHorizontalBox(Tab);
			TabSlot->SetPadding(FMargin(0.f, 0.f, BuildTabGap, 0.f));
			TabSlot->SetVerticalAlignment(VAlign_Center);
		}
	};
	auto AddTool = [this, &AddTab](FText const& Text, TFunction<void()> Picked)
	{
		UGeoListRowWidget* Tool = MakeRow(EGeoListRowTint::Normal, MoveTemp(Picked));
		if (Tool)
		{
			Tool->AddTextColumn(Text, 0.f);
		}

		AddTab(Tool);
	};

	for (int32 BuildIndex = 0; BuildIndex < Builds.Num(); ++BuildIndex)
	{
		bool const bInPlay = BuildIndex == ActiveBuild;
		if (bInPlay && bRenamingBuild)
		{
			UGeoEditableTextBox* NameField =
				WidgetTree->ConstructWidget<UGeoEditableTextBox>(UGeoEditableTextBox::StaticClass());
			NameField->SetText(FText::FromString(Builds[BuildIndex].Name));
			NameField->SetHintText(FText::Format(BuildNameFormat, BuildIndex + 1));
			NameField->SetSelectAllTextWhenFocused(true);
			NameField->OnTextCommitted.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleBuildNameCommitted);
			UGeoFrame* FieldFrame = WidgetTree->ConstructWidget<UGeoFrame>(UGeoFrame::StaticClass());
			FieldFrame->SetFrameStyle(Theme ? Theme->FieldFrameStyle : nullptr);
			FieldFrame->SetActivateOnHoverAndFocus(true);
			FieldFrame->SetContent(NameField);
			USizeBox* FieldSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			FieldSize->SetWidthOverride(BuildNameFieldWidth);
			FieldSize->SetContent(FieldFrame);
			AddTab(FieldSize);
			NameField->SetKeyboardFocus();
		}
		else
		{
			TFunction<void()> Play = [this, BuildIndex]
			{
				ChangeBuilds(
					[this, BuildIndex](UGeoGemProfileSave& ChangedProfile)
					{
						ChangedProfile.SetActiveBuild(ShownClass, BuildIndex);
					});
			};
			UGeoListRowWidget* Tab = MakeRow(bInPlay ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
											 bInPlay ? TFunction<void()>() : MoveTemp(Play));
			if (Tab && bInPlay && ClassStyle)
			{
				Tab->SetFrameTint(ClassStyle->Color);
				UGeoShape* Shape = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
				Shape->Size = BuildTabShapeSize;
				Shape->SetClassShape(*ClassStyle);
				Tab->AddColumn(Shape, 0.f);
			}
			if (Tab)
			{
				Tab->AddTextColumn(GetBuildName(Builds[BuildIndex], BuildIndex), 0.f);
			}

			AddTab(Tab);
		}

		if (bInPlay && !bRenamingBuild)
		{
			AddTool(RenameBuildText,
					[this]
					{
						bRenamingBuild = true;
						Refresh();
					});
		}
		if (bInPlay && !bRenamingBuild && Builds.Num() > 1)
		{
			AddTool(RemoveBuildText,
					[this]
					{
						ChangeBuilds(
							[this](UGeoGemProfileSave& ChangedProfile)
							{
								ChangedProfile.RemoveActiveBuild(ShownClass);
							});
					});
		}
	}

	if (Builds.Num() < GeoGem::MaxBuilds)
	{
		AddTool(AddBuildText,
				[this]
				{
					ChangeBuilds(
						[this](UGeoGemProfileSave& ChangedProfile)
						{
							ChangedProfile.AddBuild(ShownClass);
						});
				});
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FText UGeoGemLoadoutWidget::GetBuildName(FGeoGemBuild const& Build, int32 const BuildIndex) const
{
	return Build.Name.IsEmpty() ? FText::Format(BuildNameFormat, BuildIndex + 1)
								: FText::FromString(Build.Name.ToUpper());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ChangeBuilds(TFunctionRef<void(UGeoGemProfileSave&)> Change)
{
	if (UGeoGemProfileSave* Profile = GetProfile())
	{
		Change(*Profile);
		PickedGem = NAME_None;
		PickedSocket = INDEX_NONE;
		bRenamingBuild = false;
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleBuildNameCommitted(FText const& Text, ETextCommit::Type const CommitMethod)
{
	if (bRenamingBuild && CommitMethod == ETextCommit::OnCleared)
	{
		bRenamingBuild = false;
		Refresh();
	}
	else if (bRenamingBuild)
	{
		ChangeBuilds(
			[this, &Text](UGeoGemProfileSave& ChangedProfile)
			{
				ChangedProfile.RenameActiveBuild(ShownClass, Text.ToString());
			});
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowLevel(UGeoGemProfileSave const& Profile)
{
	int32 const Level = Profile.GetClassLevel(ShownClass);
	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelFormat, Level, GeoGem::MaxClassLevel));
	}

	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
	if (LevelPips)
	{
		LevelPips->SetFill(static_cast<float>(Level) / GeoGem::MaxClassLevel);
		if (ClassStyle)
		{
			LevelPips->SetFillTint(ClassStyle->Color);
		}
	}

	if (NextText)
	{
		TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
		int32 NextLevel = GeoGem::MaxClassLevel + 1;
		for (FGeoGemSocket const& Socket : Sockets)
		{
			NextLevel = Socket.UnlockLevel > Level ? FMath::Min(NextLevel, Socket.UnlockLevel) : NextLevel;
		}
		int32 const NextSockets = Algo::CountIf(Sockets,
												[NextLevel](FGeoGemSocket const& Socket)
												{
													return Socket.UnlockLevel == NextLevel;
												});
		NextText->SetText(NextSockets > 0 ? FText::Format(NextFormat, NextLevel, NextSockets)
										  : FText::Format(AllOpenFormat, Sockets.Num()));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<int32> UGeoGemLoadoutWidget::GetTargetSockets(UGeoGemProfileSave const& Profile,
													  UGeoGemCatalog const& Catalog) const
{
	TArray<int32> Targets;
	for (int32 Index = 0; !PickedGem.IsNone() && Index < GeoGem::GetSockets().Num(); ++Index)
	{
		if (Profile.CanEquip(Catalog, ShownClass, Index, PickedGem))
		{
			Targets.Add(Index);
		}
	}
	return Targets;
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoGemLoadoutWidget::GetFillCount(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog) const
{
	TArray<FName> const Slotted = Profile.GetLoadout(ShownClass).Sockets;
	int32 const EmptyTargets = Algo::CountIf(GetTargetSockets(Profile, Catalog),
											 [&Slotted](int32 const Index)
											 {
												 return Slotted[Index].IsNone();
											 });
	int32 const MostCopies = Catalog.FindTier(PickedGem) == EGeoGemTier::Core
							   ? 1
							   : Profile.GetFreeCount(PickedGem, ShownClass);
	return FMath::Min(EmptyTargets, MostCopies);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowSockets(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	TArray<FName> const Slotted = Profile.GetLoadout(ShownClass).Sockets;
	TArray<int32> const Targets = GetTargetSockets(Profile, Catalog);
	int32 const Level = Profile.GetClassLevel(ShownClass);
	for (int32 Index = 0; Index < SocketButtons.Num(); ++Index)
	{
		UGeoGemGlyph* Glyph = SocketButtons[Index]->GetGlyph();
		FGeoGemInfo const* Gem = Catalog.Find(Slotted[Index]);
		TOptional<EGeoGemTier> const GemTier = Catalog.FindTier(Slotted[Index]);
		if (Gem && GemTier)
		{
			Glyph->SetGem(*GemTier, Gem->Color.GetColor(1.f));
		}
		else
		{
			Glyph->SetSocket(Sockets[Index].Tier, Sockets[Index].UnlockLevel > Level, Sockets[Index].UnlockLevel);
		}
		Glyph->SetHighlighted(Index == PickedSocket || Targets.Contains(Index));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowFilters(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	if (!TierFilterBox)
	{
		return;
	}

	TMap<EGeoGemTier, int32> OwnedPerTier;
	int32 OwnedTotal = 0;
	for (EGeoGemTier const Tier : GetTiers())
	{
		FGeoGemList const* List = Catalog.GemsByTier.Find(Tier);
		for (FGeoGemInfo const& Gem : List ? List->Gems : TArray<FGeoGemInfo>())
		{
			OwnedPerTier.FindOrAdd(Tier) += Profile.GetOwnedCount(Gem.Id);
			OwnedTotal += Profile.GetOwnedCount(Gem.Id);
		}
	}

	TierFilterBox->ClearChildren();
	auto AddFilter = [this](TOptional<EGeoGemTier> const Filter, FText const& Name, int32 const Owned)
	{
		UGeoListRowWidget* Tab = MakeRow(Filter == TierFilter ? EGeoListRowTint::Selected : EGeoListRowTint::Header,
										 [this, Filter]
										 {
											 TierFilter = Filter;
											 Refresh();
										 });
		if (Tab)
		{
			Tab->AddTextColumn(FText::Format(FilterFormat, Name, Owned), 0.f);
			TierFilterBox->AddChildToHorizontalBox(Tab)->SetPadding(FMargin(0.f, 0.f, FilterGap, 0.f));
		}
	};
	AddFilter(TOptional<EGeoGemTier>(), AllFilterText, OwnedTotal);
	for (EGeoGemTier const Tier : GetTiers())
	{
		AddFilter(Tier, GetTierName(Tier, true), OwnedPerTier.FindRef(Tier));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowStacks(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	bool const bHadFocus = StackGrid->HasFocusedDescendants();
	UGeoListRowWidget* PickedRow = nullptr;
	StackGrid->ClearChildren();

	int32 StackIndex = 0;
	for (int32 TierIndex = GetTiers().Num() - 1; TierIndex >= 0; --TierIndex)
	{
		EGeoGemTier const Tier = GetTiers()[TierIndex];
		FGeoGemList const* List = Catalog.GemsByTier.Find(Tier);
		for (FGeoGemInfo const& Gem : TierFilter.Get(Tier) == Tier && List ? List->Gems : TArray<FGeoGemInfo>())
		{
			int32 const Owned = Profile.GetOwnedCount(Gem.Id);
			FName const GemId = Gem.Id;
			UGeoListRowWidget* Row = Owned > 0 ? MakeRow(GemId == PickedGem ? EGeoListRowTint::Selected
																			: EGeoListRowTint::Normal,
														 [this, GemId]
														 {
															 PickStack(GemId);
														 })
											   : nullptr;
			if (Row)
			{
				Row->AddColumn(MakeGemGlyph(Catalog, GemId, StackGlyphSize), 0.f);
				UVerticalBox* Names = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
				Names->AddChildToVerticalBox(MakeText(EGeoTextRole::Body, Gem.DisplayName.ToUpper()));
				Names->AddChildToVerticalBox(MakeText(
					EGeoTextRole::Label,
					FText::Format(StackSubFormat, GetTierName(Tier, false), Profile.GetFreeCount(GemId, ShownClass))));
				Row->AddColumn(Names, 1.f);
				Row->AddColumn(MakeText(EGeoTextRole::Mono, FText::Format(StackCountFormat, Owned)), 0.f);
				StackGrid->AddChildToUniformGrid(Row, StackIndex / StackColumns, StackIndex % StackColumns);
				PickedRow = GemId == PickedGem ? Row : PickedRow;
				++StackIndex;
			}
		}
	}

	if (bHadFocus && PickedRow)
	{
		PickedRow->FocusRow();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowDetail(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	TArray<FName> const Slotted = Profile.GetLoadout(ShownClass).Sockets;
	bool const bSocketPicked = Sockets.IsValidIndex(PickedSocket);
	FName const ShownGem = bSocketPicked ? Slotted[PickedSocket] : PickedGem;
	FGeoGemInfo const* Gem = Catalog.Find(ShownGem);
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(ShownGem);

	SetShown(DetailBox, Gem && Tier);
	SetShown(HintText, !Gem || !Tier);
	if (Gem && Tier)
	{
		ShowGemDetail(Profile, Catalog, *Gem, *Tier, bSocketPicked);
	}
	else if (HintText)
	{
		UGeoUITheme const* Theme = UGeoUITheme::Get();
		FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
		FText Hint = HintNothingPicked;
		if (bSocketPicked)
		{
			FGeoGemSocket const& Socket = Sockets[PickedSocket];
			FText const TierName = GetTierName(Socket.Tier, false).ToLower();
			Hint = Socket.UnlockLevel > Profile.GetClassLevel(ShownClass)
					 ? FText::Format(HintLockedSocket, TierName,
									 ClassStyle ? ClassStyle->Name.ToLower() : FText::GetEmpty(), Socket.UnlockLevel)
					 : FText::Format(HintEmptySocket, TierName);
		}
		HintText->SetText(Hint);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowGemDetail(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog,
										 FGeoGemInfo const& Gem, EGeoGemTier const Tier, bool const bSocketPicked)
{
	TArray<FName> const Slotted = Profile.GetLoadout(ShownClass).Sockets;
	int32 const Owned = Profile.GetOwnedCount(Gem.Id);
	int32 const Free = Profile.GetFreeCount(Gem.Id, ShownClass);
	if (DetailGlyph)
	{
		DetailGlyph->SetGem(Tier, Gem.Color.GetColor(1.f));
	}
	if (DetailTierText)
	{
		int32 const TierNumber = GetTiers().IndexOfByKey(Tier) + 1;
		DetailTierText->SetText(
			FText::Format(bSocketPicked ? SocketTierFormat : TierFormat, TierNumber, GetTierName(Tier, false)));
	}
	if (DetailNameText)
	{
		DetailNameText->SetText(Gem.DisplayName.ToUpper());
	}
	if (DetailEffectText)
	{
		DetailEffectText->SetText(Gem.GetEffectText());
	}
	if (DetailCountsText)
	{
		DetailCountsText->SetText(FText::Format(CountsFormat, Owned, Owned - Free, Free));
	}

	int32 const FillCount = bSocketPicked ? 0 : GetFillCount(Profile, Catalog);
	SetShown(EquipButton, !bSocketPicked);
	SetShown(FillButton, !bSocketPicked);
	SetShown(UnequipButton, bSocketPicked);
	if (EquipButton)
	{
		EquipButton->SetIsEnabled(FillCount > 0);
	}
	if (FillButton)
	{
		FillButton->SetLabel(FText::Format(FillFormat, FillCount));
		FillButton->SetIsEnabled(FillCount > 1);
	}
	if (DetailNoteText)
	{
		bool const bCoreHeld = Tier == EGeoGemTier::Core && Slotted.Contains(Gem.Id);
		DetailNoteText->SetText(bSocketPicked || FillCount > 0 ? FText::GetEmpty()
								: bCoreHeld					   ? NoteCoreOnce
								: Free <= 0					   ? NoteNoFreeCopy
															   : FText::Format(NoteNoSocket, GetTierName(Tier, false).ToLower()));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowTotals(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	TArray<FName> const Slotted = Profile.GetLoadout(ShownClass).Sockets;
	int32 const Level = Profile.GetClassLevel(ShownClass);
	if (TotalsTitleText)
	{
		UGeoUITheme const* Theme = UGeoUITheme::Get();
		FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
		int32 const Filled = Algo::CountIf(Slotted,
										   [](FName const GemId)
										   {
											   return !GemId.IsNone();
										   });
		int32 const Open = Algo::CountIf(Sockets,
										 [Level](FGeoGemSocket const& Socket)
										 {
											 return Socket.UnlockLevel <= Level;
										 });
		TotalsTitleText->SetText(FText::Format(TotalsFormat, ClassStyle ? ClassStyle->Name : FText::GetEmpty(),
											   Filled, Open));
	}
	if (TotalsBox)
	{
		TotalsBox->ClearChildren();
	}

	for (EGeoGemTier const Tier : GetTiers())
	{
		FGeoGemList const* List = Catalog.GemsByTier.Find(Tier);
		for (FGeoGemInfo const& Gem : List ? List->Gems : TArray<FGeoGemInfo>())
		{
			int32 const Count = static_cast<int32>(Algo::Count(Slotted, Gem.Id));
			if (Count > 0 && TotalsBox)
			{
				bool const bStat = Gem.MagnitudePerGem != 0.f;
				FText const Value = !bStat		? CoreTotalText
								  : Count > 1 ? FText::Format(CopiesFormat, Gem.GetMagnitudeText(Count), Count)
											  : Gem.GetMagnitudeText(Count);
				UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				Line->AddChildToHorizontalBox(MakeGemGlyph(Catalog, Gem.Id, TotalsGlyphSize))
					->SetVerticalAlignment(VAlign_Center);
				UHorizontalBoxSlot* const LabelSlot =
					Line->AddChildToHorizontalBox(MakeText(EGeoTextRole::Body, bStat ? Gem.Effect : Gem.DisplayName));
				LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				LabelSlot->SetPadding(FMargin(TotalsLabelGap, 0.f));
				LabelSlot->SetVerticalAlignment(VAlign_Center);
				Line->AddChildToHorizontalBox(MakeText(EGeoTextRole::Mono, Value))->SetVerticalAlignment(VAlign_Center);
				TotalsBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 0.f, 0.f, TotalsLineGap));
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::PickSocket(int32 const SocketIndex)
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	bool const bEquip = Profile && Catalog && !PickedGem.IsNone()
					 && Profile->Equip(*Catalog, ShownClass, SocketIndex, PickedGem);
	if (bEquip)
	{
		CommitChanges();
	}
	else
	{
		PickedGem = NAME_None;
		PickedSocket = SocketIndex;
		Refresh();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::PickStack(FName const GemId)
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	bool const bEquip = Profile && Catalog && PickedSocket != INDEX_NONE
					 && Profile->Equip(*Catalog, ShownClass, PickedSocket, GemId);
	if (bEquip)
	{
		CommitChanges();
	}
	else
	{
		PickedGem = GemId;
		PickedSocket = INDEX_NONE;
		Refresh();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleEquip()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!Profile || !Catalog)
	{
		return;
	}

	TArray<FName> const Slotted = Profile->GetLoadout(ShownClass).Sockets;
	TArray<int32> const Targets = GetTargetSockets(*Profile, *Catalog);
	int32 const* EmptyTarget = Targets.FindByPredicate(
		[&Slotted](int32 const Index)
		{
			return Slotted[Index].IsNone();
		});
	if (EmptyTarget && Profile->Equip(*Catalog, ShownClass, *EmptyTarget, PickedGem))
	{
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleFill()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (Profile && Catalog && Profile->FillEmptySockets(*Catalog, ShownClass, PickedGem) > 0)
	{
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleUnequip()
{
	UGeoGemProfileSave* Profile = GetProfile();
	if (Profile && GeoGem::GetSockets().IsValidIndex(PickedSocket))
	{
		Profile->Unequip(ShownClass, PickedSocket);
		CommitChanges();
	}
}
