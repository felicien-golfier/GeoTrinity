// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Gems/GeoGemLoadoutWidget.h"

#include "Algo/AnyOf.h"
#include "Algo/Count.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemProfileSave.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Menu/Frame/GeoButton.h"
#include "HUD/Menu/Frame/GeoMenuButton.h"
#include "HUD/Menu/Frame/GeoMessageSubsystem.h"
#include "HUD/Menu/Gems/GeoGemSocketButton.h"
#include "HUD/Menu/List/GeoTableWidget.h"
#include "HUD/Style/Frame/GeoFrame.h"
#include "HUD/Style/GeoThemedInputs.h"
#include "HUD/Style/GeoUITheme.h"
#include "HUD/Style/Glyph/GeoEchoHalo.h"
#include "HUD/Style/Glyph/GeoGemGlyph.h"
#include "HUD/Style/Glyph/GeoIconImage.h"
#include "HUD/Style/Glyph/GeoShape.h"
#include "Tool/Visual/GeoColor.h"
#include "Tool/Visual/GeoIcon.h"

bool UGeoGemLoadoutWidget::bEchoFlared = false;

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
	if (UnequipTierButton)
	{
		UnequipTierButton->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleUnequipTier);
	}
	SetVisibility(ESlateVisibility::Visible);
	Refresh();
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoGemLoadoutWidget::NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent)
{
	FVector2D const Click = InMouseEvent.GetScreenSpacePosition();
	bool const bOnPickingPart =
		Algo::AnyOf(TArray<UWidget const*>{StackGrid, DetailBox},
					[&Click](UWidget const* Part)
					{
						return Part && Part->IsVisible() && Part->GetCachedGeometry().IsUnderLocation(Click);
					});
	bool const bDrop = !bOnPickingPart && (!PickedGem.IsNone() || PickedSocket != INDEX_NONE);
	if (bDrop)
	{
		ClearPick();
		Refresh();
	}
	if (bRenamingBuild)
	{
		EndRename();
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
	ShowFilters();
	ShowUnequipTier(*Profile, *Catalog);
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
		OuterRadius = FMath::Max(OuterRadius, Rings.FindRef(Sockets[Index].Tier).Radius);
	}

	auto AddToBoard = [this](UWidget* Widget, FVector2D const& Position, FVector2D const& Size)
	{
		UCanvasPanelSlot* const BoardSlot = Board->AddChildToCanvas(Widget);
		BoardSlot->SetAutoSize(false);
		BoardSlot->SetSize(Size);
		BoardSlot->SetPosition(Position - .5f * Size);
	};
	auto AddSocket = [this, &AddToBoard](int32 const Index, float const Size, FVector2D const& Position)
	{
		UGeoGemGlyph* Glyph = WidgetTree->ConstructWidget<UGeoGemGlyph>(UGeoGemGlyph::StaticClass());
		Glyph->SetSize(Size);
		UGeoGemSocketButton* Socket =
			WidgetTree->ConstructWidget<UGeoGemSocketButton>(UGeoGemSocketButton::StaticClass());
		Socket->InitSocket(Index, Glyph);
		Socket->OnPicked.BindUObject(this, &UGeoGemLoadoutWidget::PickSocket);
		Socket->OnHoverChanged.BindWeakLambda(this,
											  [this](int32 const SocketIndex, bool const bHovered)
											  {
												  Hover(NAME_None, bHovered ? SocketIndex : INDEX_NONE);
											  });
		AddToBoard(Socket, Position, FVector2D(Size));
		SocketButtons.Add(Socket);
	};
	auto AddCircle =
		[this, &AddToBoard](FVector2D const& Center, float const Radius, bool const bFilled, FLinearColor const& Color)
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
	float LinksExtent = 0.f;
	for (FGeoGemCluster const& Cluster : Clusters)
	{
		LinksExtent = FMath::Max(LinksExtent, Cluster.Center.GetMax());
	}

	AddToBoard(MakeClusterLinks(LinksExtent), FVector2D(.5f * LinksExtent), FVector2D(LinksExtent));

	EchoHalo = WidgetTree->ConstructWidget<UGeoEchoHalo>(UGeoEchoHalo::StaticClass());
	EchoHalo->FrameStyle = EchoHaloStyle;
	EchoHalo->SetVisibility(ESlateVisibility::HitTestInvisible);
	// Under every cluster, so the neighbouring sockets never sit beneath it.
	AddToBoard(EchoHalo, EchoPosition, FVector2D(EchoHaloSize));
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

		UTextBlock* Label =
			MakeText(EGeoTextRole::Label,
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

			AddSocket(Index, Ring.SocketSize,
					  Cluster.Center + Ring.Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
	}

	AddSocket(GetEchoIndex(), EchoSize, EchoPosition);
}

// ---------------------------------------------------------------------------------------------------------------------
FGeoGemSocket UGeoGemLoadoutWidget::GetSocket(int32 const Index)
{
	return Index == GetEchoIndex() ? FGeoGemSocket{EGeoGemTier::Core, GeoGem::EchoUnlockLevel}
								   : GeoGem::GetSockets()[Index];
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoGemLoadoutWidget::IsSocketLocked(UGeoGemProfileSave const& Profile, int32 const Index) const
{
	return Index == GetEchoIndex() ? Profile.GetEchoLevel() < 1
								   : GetSocket(Index).UnlockLevel > Profile.GetClassLevel(ShownClass);
}

// ---------------------------------------------------------------------------------------------------------------------
FName UGeoGemLoadoutWidget::GetSlottedGem(FGeoGemLoadout const& Loadout, int32 const Index)
{
	return Index == GetEchoIndex() ? Loadout.Echo : Loadout.Sockets[Index];
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoIconImage* UGeoGemLoadoutWidget::MakeClusterLinks(float const Extent)
{
	UGeoIcon* Links = NewObject<UGeoIcon>(this);
	Links->Color = FGeoColorParam(ClusterLinkColor);
	Links->ViewSize = Extent;
	FGeoIconStroke& Link = Links->Strokes.AddDefaulted_GetRef();
	for (FGeoGemCluster const& Cluster : Clusters)
	{
		Link.Points.Add(Cluster.Center);
	}

	Link.bClosed = true;
	Link.LineThickness = ClusterLinkThickness;
	Link.DashLength = ClusterLinkDash;
	Link.DashGap = ClusterLinkGap;
	UGeoIconImage* Image = WidgetTree->ConstructWidget<UGeoIconImage>(UGeoIconImage::StaticClass());
	Image->SetIcon(Links);
	Image->SetSize(Extent);
	return Image;
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
	for (EPlayerClass const PlayerClass : ClassTabOrder)
	{
		FGeoClassStyle const* ClassStyle = Theme->FindClassStyle(PlayerClass);
		UGeoListRowWidget* Tab = ClassStyle
			? MakeRow(PlayerClass == ShownClass ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
					  [this, PlayerClass]
					  {
						  ShownClass = PlayerClass;
						  ClearPick();
						  bRenamingBuild = false;
						  Refresh();
					  })
			: nullptr;
		if (Tab)
		{
			UGeoShape* Shape = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
			Shape->Size = ClassTabShapeSize;
			Shape->SetClassShape(*ClassStyle);
			Tab->AddColumn(Shape, 0.f);
			Tab->AddTextColumn(ClassStyle->Name, 0.f);
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
		InPlayShape->Color = ClassStyle->Color;
		InPlayShape->SynchronizeProperties();
	}
	if (InPlayText)
	{
		InPlayText->SetText(GetBuildName(Builds[ActiveBuild], ActiveBuild));
	}
	if (BuildCountText)
	{
		BuildCountText->SetText(FText::Format(BuildCountFormat, Builds.Num(), GeoGem::MaxBuilds));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowBuildTabs(UGeoGemProfileSave const& Profile)
{
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	if (!BuildTabBox || !Theme)
	{
		return;
	}

	TArray<FGeoGemBuild> const Builds = Profile.GetBuilds(ShownClass);
	int32 const ActiveBuild = Profile.GetActiveBuild(ShownClass);
	FGeoClassStyle const* ClassStyle = Theme->FindClassStyle(ShownClass);
	BuildTabBox->ClearChildren();
	BuildNameField = nullptr;
	auto AddTab = [this](UWidget* Tab)
	{
		UHorizontalBoxSlot* const TabSlot = BuildTabBox->AddChildToHorizontalBox(Tab);
		TabSlot->SetPadding(FMargin(0.f, 0.f, BuildTabGap, 0.f));
		TabSlot->SetVerticalAlignment(VAlign_Center);
	};
	auto AddTool = [this](UGeoListRowWidget* Tab, UWidget* Tool)
	{
		USizeBox* ToolSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ToolSize->SetWidthOverride(BuildToolSize.X);
		ToolSize->SetHeightOverride(BuildToolSize.Y);
		ToolSize->SetContent(Tool);
		Tab->AddColumn(ToolSize, 0.f);
	};

	for (int32 BuildIndex = 0; BuildIndex < Builds.Num(); ++BuildIndex)
	{
		bool const bInPlay = BuildIndex == ActiveBuild;
		TFunction<void()> Play = [this, BuildIndex]
		{
			ChangeBuilds(
				[this, BuildIndex](UGeoGemProfileSave& ChangedProfile)
				{
					ChangedProfile.SetActiveBuild(ShownClass, BuildIndex);
				});
		};
		UGeoListRowWidget* Tab = MakeRow(bInPlay ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
										 bInPlay ? TFunction<void()>([] {}) : MoveTemp(Play));
		if (Tab)
		{
			Tab->SetClickMethod(EButtonClickMethod::MouseDown);
		}
		if (Tab && bInPlay)
		{
			Tab->OnDoubleClicked.AddWeakLambda(this,
											   [this]
											   {
												   StartRename();
											   });
		}
		if (Tab && bInPlay && ClassStyle)
		{
			Tab->SetFrameTint(ClassStyle->Color);
			UGeoShape* Shape = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
			Shape->Size = BuildTabShapeSize;
			Shape->SetClassShape(*ClassStyle);
			Tab->AddColumn(Shape, 0.f);
		}
		if (Tab && bInPlay && bRenamingBuild)
		{
			BuildNameField = WidgetTree->ConstructWidget<UGeoEditableTextBox>(UGeoEditableTextBox::StaticClass());
			BuildNameField->SetPaddingOverride(BuildNameFieldPadding);
			BuildNameField->SetText(FText::FromString(Builds[BuildIndex].Name.ToUpper()));
			BuildNameField->SetHintText(FText::Format(BuildNameFormat, BuildIndex + 1));
			BuildNameField->OnTextChanged.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleBuildNameChanged);
			BuildNameField->OnTextCommitted.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleBuildNameCommitted);
			UGeoFrame* FieldFrame = WidgetTree->ConstructWidget<UGeoFrame>(UGeoFrame::StaticClass());
			FieldFrame->SetFrameStyle(Theme->FieldFrameStyle);
			FieldFrame->SetActivateOnHoverAndFocus(true);
			FieldFrame->SetContent(BuildNameField);
			USizeBox* FieldSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			FieldSize->SetWidthOverride(BuildNameFieldWidth);
			FieldSize->SetContent(FieldFrame);
			Tab->AddColumn(FieldSize, 0.f);
		}
		else if (Tab && bInPlay)
		{
			Tab->AddTextColumn(GetBuildName(Builds[BuildIndex], BuildIndex), 0.f);
		}
		else if (Tab)
		{
			UTextBlock* Name = Tab->MakeColumnText(GetBuildName(Builds[BuildIndex], BuildIndex));
			Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
			USizeBox* NameSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			NameSize->SetMaxDesiredWidth(BuildTabNameMaxWidth);
			NameSize->SetContent(Name);
			Tab->AddColumn(NameSize, 0.f);
		}
		if (Tab && bInPlay && !bRenamingBuild)
		{
			UGeoButton* Rename = MakeIconButton(RenameBuildIcon, RenameBuildIconSize, Theme->IconButtonStyle, true);
			Rename->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleRenameBuild);
			AddTool(Tab, Rename);
		}
		if (Tab && bInPlay && Builds.Num() > 1)
		{
			UGeoButton* Remove = MakeIconButton(RemoveBuildIcon, RemoveBuildIconSize, Theme->IconButtonStyle, true);
			Remove->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleRemoveBuild);
			AddTool(Tab, Remove);
		}
		if (Tab)
		{
			AddTab(Tab);
		}
	}

	if (Builds.Num() < GeoGem::MaxBuilds)
	{
		UGeoButton* Add = MakeIconButton(AddBuildIcon, AddBuildIconSize, Theme->OutlinedIconButtonStyle, false);
		Add->OnClicked.AddUniqueDynamic(this, &UGeoGemLoadoutWidget::HandleAddBuild);
		AddTab(Add);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UGeoButton* UGeoGemLoadoutWidget::MakeIconButton(UGeoIcon const* Icon, float const IconSize, FButtonStyle const& Style,
												 bool const bTintWithForeground)
{
	ensureMsgf(Icon, TEXT("%hs: an icon of the build tabs is not set on %s"), __FUNCTION__, *GetName());
	UGeoIconImage* Image = WidgetTree->ConstructWidget<UGeoIconImage>(UGeoIconImage::StaticClass());
	Image->SetIcon(Icon);
	Image->SetSize(IconSize);
	Image->SetTintWithForeground(bTintWithForeground);
	UGeoButton* Button = WidgetTree->ConstructWidget<UGeoButton>(UGeoButton::StaticClass());
	Button->SetStyle(Style);
	Button->SetClickMethod(EButtonClickMethod::MouseDown);
	UButtonSlot* const ImageSlot = Cast<UButtonSlot>(Button->SetContent(Image));
	ImageSlot->SetPadding(FMargin(0.f));
	ImageSlot->SetHorizontalAlignment(HAlign_Center);
	ImageSlot->SetVerticalAlignment(VAlign_Center);
	return Button;
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
		ClearPick();
		bRenamingBuild = false;
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::StartRename()
{
	UGeoGemProfileSave const* Profile = GetProfile();
	bRenamingBuild = true;
	if (Profile)
	{
		ShowBuildTabs(*Profile);
	}
	if (BuildNameField)
	{
		BuildNameField->SetKeyboardFocus();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::EndRename()
{
	UGeoGemProfileSave const* Profile = GetProfile();
	bRenamingBuild = false;
	if (Profile)
	{
		CommitProfile();
		ShowBuildTabs(*Profile);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleRenameBuild()
{
	StartRename();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleRemoveBuild()
{
	ChangeBuilds(
		[this](UGeoGemProfileSave& ChangedProfile)
		{
			bool const bLast = ChangedProfile.GetBuilds(ShownClass).Num() <= 1;
			ChangedProfile.RemoveActiveBuild(ShownClass);
			UGeoMessageSubsystem::Post(this, bLast ? MsgLastBuild : MsgBuildRemoved,
									   bLast ? EGeoColor::Damage : EGeoColor::Neutral);
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleAddBuild()
{
	ChangeBuilds(
		[this](UGeoGemProfileSave& ChangedProfile)
		{
			bool const bAdded = ChangedProfile.AddBuild(ShownClass);
			UGeoMessageSubsystem::Post(this, bAdded ? MsgBuildAdded : FText::Format(MsgMaxBuilds, GeoGem::MaxBuilds),
									   bAdded ? EGeoColor::Heal : EGeoColor::Damage);
		});
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleBuildNameChanged(FText const& Text)
{
	FString const Typed = Text.ToString();
	FString const Name = Typed.ToUpper().Left(GeoGem::MaxBuildNameLength);
	UGeoGemProfileSave* Profile = GetProfile();
	if (BuildNameField && Name != Typed)
	{
		BuildNameField->SetText(FText::FromString(Name));
	}
	if (Profile)
	{
		Profile->RenameActiveBuild(ShownClass, Name);
		ShowInPlay(*Profile);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleBuildNameCommitted(FText const& /*Text*/, ETextCommit::Type /*CommitMethod*/)
{
	if (bRenamingBuild)
	{
		EndRename();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowLevel(UGeoGemProfileSave const& Profile)
{
	int32 const Level = Profile.GetClassLevel(ShownClass);
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Level));
	}
	if (LevelMaxText)
	{
		FText const EchoLevel =
			Profile.GetEchoLevel() >= 1 ? FText::Format(EchoLevelFormat, Profile.GetEchoLevel()) : FText::GetEmpty();
		LevelMaxText->SetText(
			FText::Join(FText::GetEmpty(), FText::Format(LevelMaxFormat, GeoGem::MaxClassLevel), EchoLevel));
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
	bool const bCore = Catalog.FindTier(PickedGem) == EGeoGemTier::Core;
	bool const bHeld = Profile.GetLoadout(ShownClass).Sockets.Contains(PickedGem);
	int32 const MostCopies = bCore ? (bHeld ? 0 : 1) : Profile.GetFreeCount(PickedGem, ShownClass);
	return FMath::Min(GetTargetSockets(Profile, Catalog).Num(), MostCopies);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowSockets(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	FGeoGemLoadout const Loadout = Profile.GetLoadout(ShownClass);
	TArray<int32> const Targets = GetTargetSockets(Profile, Catalog);
	bool const bEchoTarget = !PickedGem.IsNone() && Profile.CanEcho(Catalog, ShownClass, PickedGem);
	for (int32 Index = 0; Index < SocketButtons.Num(); ++Index)
	{
		UGeoGemGlyph* Glyph = SocketButtons[Index]->GetGlyph();
		FName const GemId = GetSlottedGem(Loadout, Index);
		FGeoGemInfo const* Gem = Catalog.Find(GemId);
		TOptional<EGeoGemTier> const GemTier = Catalog.FindTier(GemId);
		FGeoGemSocket const Socket = GetSocket(Index);
		if (Gem && GemTier)
		{
			Glyph->SetGem(*GemTier, Gem->Color.GetColor(1.f));
		}
		else if (Index == GetEchoIndex())
		{
			Glyph->SetEchoSocket(IsSocketLocked(Profile, Index), Socket.UnlockLevel);
		}
		else
		{
			Glyph->SetSocket(Socket.Tier, IsSocketLocked(Profile, Index), Socket.UnlockLevel);
		}
		bool const bTarget = Index == GetEchoIndex() ? bEchoTarget : Targets.Contains(Index);
		Glyph->SetHighlighted(Index == PickedSocket || bTarget);
	}

	ShowEchoHalo(Profile, Catalog);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowEchoHalo(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	if (EchoHalo)
	{
		FGeoGemInfo const* Gem = Catalog.Find(Profile.GetEcho(ShownClass));
		bool const bLocked = IsSocketLocked(Profile, GetEchoIndex());
		bool const bEchoTarget = !PickedGem.IsNone() && Profile.CanEcho(Catalog, ShownClass, PickedGem);
		EchoHalo->SetLocked(bLocked);
		EchoHalo->SetColor(Gem ? Gem->Color.GetColor(1.f) : EchoHaloOpenColor);
		EchoHalo->SetActive(PickedSocket == GetEchoIndex() || HoveredSocket == GetEchoIndex() || bEchoTarget);
		if (!bLocked && !bEchoFlared)
		{
			EchoHalo->PlayFlare();
			bEchoFlared = true;
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowFilters()
{
	if (!TierFilterBox)
	{
		return;
	}

	TierFilterBox->ClearChildren();
	auto AddFilter = [this](TOptional<EGeoGemTier> const Filter, FText const& Name)
	{
		UGeoListRowWidget* Tab = MakeRow(
			Filter == TierFilter ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
			[this, Filter]
			{
				TierFilter = Filter;
				Refresh();
			},
			TabRowClass);
		if (Tab && TierFilterBox->HasAnyChildren())
		{
			TierFilterBox->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))
				->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		if (Tab)
		{
			Tab->AddTextColumn(Name, 0.f);
			TierFilterBox->AddChildToHorizontalBox(Tab);
		}
	};
	AddFilter(TOptional<EGeoGemTier>(), AllFilterText);
	for (EGeoGemTier const Tier : GetTiers())
	{
		AddFilter(Tier, GetTierName(Tier, true));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowUnequipTier(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	if (UnequipTierButton)
	{
		FGeoGemLoadout const Loadout = Profile.GetLoadout(ShownClass);
		TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
		auto IsFiltered = [this](TOptional<EGeoGemTier> const Tier)
		{
			return Tier && (!TierFilter || *TierFilter == *Tier);
		};
		bool bHoldsGem = IsFiltered(Catalog.FindTier(Loadout.Echo));
		for (int32 SocketIndex = 0; SocketIndex < Sockets.Num() && !bHoldsGem; ++SocketIndex)
		{
			bHoldsGem = !Loadout.Sockets[SocketIndex].IsNone() && IsFiltered(Sockets[SocketIndex].Tier);
		}

		UnequipTierButton->SetLabel(TierFilter ? FText::Format(UnequipTierFormat, GetTierName(*TierFilter, true))
											   : UnequipAllText);
		UnequipTierButton->SetIsEnabled(bHoldsGem);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowStacks(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	bool const bHadFocus = StackGrid->HasFocusedDescendants();
	FGeoGemLoadout const Loadout = Profile.GetLoadout(ShownClass);
	UGeoListRowWidget* PickedRow = nullptr;
	HoveredGem = NAME_None;
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
			bool const bEchoed = Loadout.Echo == GemId;
			UGeoListRowWidget* Row = Owned > 0
				? MakeRow(GemId == PickedGem ? EGeoListRowTint::Selected : EGeoListRowTint::Normal,
						  [this, GemId]
						  {
							  PickStack(GemId);
						  })
				: nullptr;
			if (Row)
			{
				Row->OnHoverChanged.AddWeakLambda(this,
												  [this, GemId](bool const bHovered)
												  {
													  Hover(bHovered ? GemId : NAME_None, INDEX_NONE);
												  });
				Row->AddColumn(MakeGemGlyph(Catalog, GemId, StackGlyphSize), 0.f);
				UVerticalBox* Names = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
				Names->AddChildToVerticalBox(MakeText(EGeoTextRole::Body, Gem.DisplayName.ToUpper()));
				Names->AddChildToVerticalBox(MakeText(EGeoTextRole::Label,
													  bEchoed
														  ? FText::Format(StackEchoedFormat, GetTierName(Tier, false))
														  : FText::Format(StackSubFormat, GetTierName(Tier, false),
																		  Profile.GetFreeCount(GemId, ShownClass))));
				Row->AddColumn(Names, 1.f);
				Row->AddColumn(MakeText(EGeoTextRole::Mono, FText::Format(CountFormat, Owned)), 0.f);
				if (Loadout.Sockets.Contains(GemId) || bEchoed)
				{
					UGeoShape* Mark = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
					Mark->Sides = 4;
					Mark->Rotation = 45.f;
					Mark->Size = InBuildMarkSize;
					Mark->bFilled = true;
					Mark->Color = InBuildMarkColor;
					Row->AddColumn(Mark, 0.f);
				}
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
	FGeoGemLoadout const Loadout = Profile.GetLoadout(ShownClass);
	bool const bSocketPicked = PickedSocket != INDEX_NONE;
	bool const bSocketHovered = HoveredSocket != INDEX_NONE && !GetSlottedGem(Loadout, HoveredSocket).IsNone();
	bool const bStackHovered = !HoveredGem.IsNone();
	bool const bPreview = bSocketHovered ? HoveredSocket != PickedSocket : bStackHovered && HoveredGem != PickedGem;
	FName const PickedShown = bSocketPicked ? GetSlottedGem(Loadout, PickedSocket) : PickedGem;
	FName const ShownGem = bSocketHovered ? GetSlottedGem(Loadout, HoveredSocket)
		: bStackHovered					  ? HoveredGem
										  : PickedShown;
	int32 const ShownSocket = bSocketHovered ? HoveredSocket
		: bStackHovered || !bSocketPicked	 ? INDEX_NONE
											 : PickedSocket;
	FGeoGemInfo const* Gem = Catalog.Find(ShownGem);
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(ShownGem);

	SetShown(DetailBox, Gem && Tier);
	SetShown(HintText, !Gem || !Tier);
	if (Gem && Tier)
	{
		ShowGemDetail(Profile, Catalog, *Gem, *Tier, ShownSocket, bPreview);
	}
	else if (HintText)
	{
		UGeoUITheme const* Theme = UGeoUITheme::Get();
		FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
		FText Hint = HintNothingPicked;
		if (bSocketPicked)
		{
			FGeoGemSocket const Socket = GetSocket(PickedSocket);
			bool const bEcho = PickedSocket == GetEchoIndex();
			FText const TierName = GetTierName(Socket.Tier, false).ToLower();
			FText const ClassName = ClassStyle ? ClassStyle->Name.ToLower() : FText::GetEmpty();
			bool const bLocked = IsSocketLocked(Profile, PickedSocket);
			Hint = bEcho  ? (bLocked ? FText::Format(HintLockedEcho, Socket.UnlockLevel) : HintEmptyEcho)
				: bLocked ? FText::Format(HintLockedSocket, TierName, ClassName, Socket.UnlockLevel)
						  : FText::Format(HintEmptySocket, TierName);
		}
		HintText->SetText(Hint);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowGemDetail(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog,
										 FGeoGemInfo const& Gem, EGeoGemTier const Tier, int32 const ShownSocket,
										 bool const bPreview)
{
	FGeoGemLoadout const Loadout = Profile.GetLoadout(ShownClass);
	TArray<FName> const& Slotted = Loadout.Sockets;
	bool const bFromSocket = ShownSocket != INDEX_NONE;
	bool const bFromEcho = ShownSocket == GetEchoIndex();
	bool const bEchoed = Loadout.Echo == Gem.Id;
	int32 const Owned = Profile.GetOwnedCount(Gem.Id);
	int32 const Free = Profile.GetFreeCount(Gem.Id, ShownClass);
	if (DetailGlyph)
	{
		DetailGlyph->SetGem(Tier, Gem.Color.GetColor(1.f));
	}
	if (DetailTierText)
	{
		int32 const TierNumber = GetTiers().IndexOfByKey(Tier) + 1;
		DetailTierText->SetText(bFromEcho ? FText::Format(EchoTierFormat, GetTierName(Tier, false))
										  : FText::Format(bFromSocket ? SocketTierFormat : TierFormat, TierNumber,
														  GetTierName(Tier, false)));
	}
	if (DetailNameText)
	{
		DetailNameText->SetText(Gem.DisplayName.ToUpper());
	}
	if (DetailEffectText)
	{
		float const Copies = bFromEcho ? Catalog.GetEchoCopies(Tier, Profile.GetEchoLevel()) : 1.f;
		DetailEffectText->SetText(Gem.GetEffectText(Copies));
	}
	if (DetailCountsText && bFromEcho)
	{
		int32 const EchoLevel = Profile.GetEchoLevel();
		DetailCountsText->SetText(EchoLevel >= Catalog.EchoMaxLevel
									  ? FText::Format(EchoMaxCountsFormat, EchoLevel)
									  : FText::Format(EchoCountsFormat, EchoLevel, Profile.GetEchoXp(),
													  Catalog.GetEchoXpForLevel(EchoLevel + 1)));
	}
	else if (DetailCountsText)
	{
		DetailCountsText->SetText(bFromSocket ? FText::Format(SocketCountsFormat, Owned, Free)
											  : FText::Format(CountsFormat, Owned, Owned - Free, Free));
	}

	int32 const FillCount = GetFillCount(Profile, Catalog);
	SetShown(EquipButton, true);
	SetShown(FillButton, true);
	bool const bEmptyTarget = Algo::AnyOf(GetTargetSockets(Profile, Catalog),
										  [&Slotted](int32 const Index)
										  {
											  return Slotted[Index].IsNone();
										  });
	if (EquipButton)
	{
		EquipButton->SetLabel(EquipText);
		EquipButton->SetIsEnabled(FillCount > 0 && bEmptyTarget);
	}
	if (FillButton)
	{
		FillButton->SetLabel(FillText);
		FillButton->SetIsEnabled(FillCount > 0);
	}
	if (DetailNoteText)
	{
		DetailNoteText->SetText(bFromSocket || bPreview ? FText::GetEmpty()
									: bEchoed			? NoteEchoed
									: FillCount > 0		? FText::GetEmpty()
									: Free <= 0			? NoteNoFreeCopy
												: FText::Format(NoteNoSocket, GetTierName(Tier, false).ToLower()));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ShowTotals(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog)
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	TArray<FName> const Slotted = Profile.GetLoadout(ShownClass).Sockets;
	FName const Echo = Profile.GetEcho(ShownClass);
	int32 const Level = Profile.GetClassLevel(ShownClass);
	if (TotalsFilledText)
	{
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
		TotalsFilledText->SetText(FText::Format(FilledFormat, Filled, Open));
	}

	auto MakeLabel = [this](FGeoGemInfo const& Gem, FText const& Name)
	{
		UGeoShape* Dot = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
		Dot->Sides = 4;
		Dot->Rotation = 45.f;
		Dot->Size = TotalsDotSize;
		Dot->bFilled = true;
		Dot->Color = Gem.Color.GetColor(1.f);
		UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Label->AddChildToHorizontalBox(Dot)->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* const NameSlot = Label->AddChildToHorizontalBox(TotalsTable->MakeCellText(Name));
		NameSlot->SetPadding(FMargin(TotalsDotGap, 0.f, 0.f, 0.f));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		return Label;
	};

	if (TotalsTable)
	{
		TotalsTable->ClearLines();
	}

	for (EGeoGemTier const Tier : GetTiers())
	{
		FGeoGemList const* List = Catalog.GemsByTier.Find(Tier);
		for (FGeoGemInfo const& Gem : List ? List->Gems : TArray<FGeoGemInfo>())
		{
			int32 const Count = static_cast<int32>(Algo::Count(Slotted, Gem.Id));
			if (Count > 0 && TotalsTable)
			{
				bool const bStat = Gem.MagnitudePerGem != 0.f;
				UWidget* Label = MakeLabel(Gem, bStat ? Gem.Effect : Gem.DisplayName);

				TArray<UWidget*> Cells = {Label};
				if (bStat)
				{
					Cells.Add(TotalsTable->MakeCellText(Gem.GetMagnitudeText(Count)));
					Cells.Add(TotalsTable->MakeCellText(FText::Format(CountFormat, Count)));
				}
				else
				{
					Cells.Add(TotalsTable->MakeCellText(CoreTotalText));
				}

				TotalsTable->AddLine(Cells);
			}
		}
	}

	FGeoGemInfo const* EchoGem = Catalog.Find(Echo);
	TOptional<EGeoGemTier> const EchoTier = Catalog.FindTier(Echo);
	if (EchoGem && EchoTier && TotalsTable)
	{
		TotalsTable->AddLine({MakeLabel(*EchoGem, EchoGem->Effect),
							  TotalsTable->MakeCellText(
								  EchoGem->GetMagnitudeText(Catalog.GetEchoCopies(*EchoTier, Profile.GetEchoLevel()))),
							  TotalsTable->MakeFittingCellText(EchoTotalText, EchoTotalShortText, 2)});
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::PickSocket(int32 const SocketIndex)
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	bool const bEcho = SocketIndex == GetEchoIndex();
	FGeoGemLoadout const Before = Profile ? Profile->GetLoadout(ShownClass) : FGeoGemLoadout();
	bool const bEquip = Profile && Catalog && !PickedGem.IsNone() && !bEcho
		&& Profile->Equip(*Catalog, ShownClass, SocketIndex, PickedGem);
	if (bEquip)
	{
		PostEquipEffects(*Catalog, Before, PickedGem);
		CommitChanges();
	}
	else if (bEcho && Profile && Catalog && Profile->CanEcho(*Catalog, ShownClass, PickedGem))
	{
		HandleEcho();
	}
	else
	{
		if (Profile && Catalog)
		{
			PostRefusal(*Catalog, GetClickRefusal(*Profile, *Catalog, SocketIndex), PickedGem, SocketIndex);
		}

		ClearPick();
		PickedSocket = SocketIndex;
		TierFilter = bEcho ? TOptional<EGeoGemTier>() : TOptional<EGeoGemTier>(GetSocket(SocketIndex).Tier);
		Refresh();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::Hover(FName const GemId, int32 const SocketIndex)
{
	HoveredGem = GemId;
	HoveredSocket = SocketIndex;
	UGeoGemProfileSave const* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (Profile && Catalog)
	{
		ShowDetail(*Profile, *Catalog);
		ShowEchoHalo(*Profile, *Catalog);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::PickStack(FName const GemId)
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	bool const bEcho = PickedSocket == GetEchoIndex();
	FGeoGemLoadout const Before = Profile ? Profile->GetLoadout(ShownClass) : FGeoGemLoadout();
	bool const bEquip = Profile && Catalog && PickedSocket != INDEX_NONE && !bEcho
		&& Profile->Equip(*Catalog, ShownClass, PickedSocket, GemId);
	bool const bEchoIt = Profile && Catalog && bEcho && Profile->CanEcho(*Catalog, ShownClass, GemId);
	if (bEquip)
	{
		PostEquipEffects(*Catalog, Before, GemId);
		CommitChanges();
	}
	else
	{
		if (Profile && Catalog && PickedSocket != INDEX_NONE && !bEchoIt)
		{
			EGeoGemRefusal const Refusal = bEcho ? Profile->GetEchoRefusal(*Catalog, ShownClass, GemId)
												 : Profile->GetEquipRefusal(*Catalog, ShownClass, PickedSocket, GemId);
			PostRefusal(*Catalog, Refusal, GemId, PickedSocket);
		}

		PickedGem = GemId;
		PickedSocket = INDEX_NONE;
		if (bEchoIt)
		{
			HandleEcho();
		}
		else
		{
			Refresh();
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::ClearPick()
{
	PickedGem = NAME_None;
	PickedSocket = INDEX_NONE;
}

// ---------------------------------------------------------------------------------------------------------------------
EGeoGemRefusal UGeoGemLoadoutWidget::GetClickRefusal(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog,
													 int32 const SocketIndex) const
{
	bool const bEcho = SocketIndex == GetEchoIndex();
	EGeoGemRefusal Refusal = EGeoGemRefusal::None;
	if (!PickedGem.IsNone())
	{
		Refusal = bEcho ? Profile.GetEchoRefusal(Catalog, ShownClass, PickedGem)
						: Profile.GetEquipRefusal(Catalog, ShownClass, SocketIndex, PickedGem);
	}
	else if (IsSocketLocked(Profile, SocketIndex))
	{
		Refusal = bEcho ? EGeoGemRefusal::EchoLocked : EGeoGemRefusal::Locked;
	}

	return Refusal;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::PostRefusal(UGeoGemCatalog const& Catalog, EGeoGemRefusal const Refusal, FName const GemId,
									   int32 const SocketIndex) const
{
	FGeoGemInfo const* Gem = Catalog.Find(GemId);
	FText const GemName = Gem ? Gem->DisplayName.ToUpper() : FText::GetEmpty();
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(ShownClass) : nullptr;
	FGeoGemSocket const Socket = GetSocket(SocketIndex);
	TOptional<FText> Message;
	switch (Refusal)
	{
	case EGeoGemRefusal::AlreadyThere:
		Message = FText::Format(MsgAlreadyThere, GemName);
		break;
	case EGeoGemRefusal::WrongTier:
		Message = FText::Format(MsgWrongTier, GemName, GetTierName(Socket.Tier, false));
		break;
	case EGeoGemRefusal::Locked:
		Message = FText::Format(MsgLocked, ClassStyle ? ClassStyle->Name : FText::GetEmpty(), Socket.UnlockLevel);
		break;
	case EGeoGemRefusal::NoFreeCopy:
		Message = FText::Format(MsgNoFreeCopy, GemName);
		break;
	case EGeoGemRefusal::EchoLocked:
		Message = FText::Format(MsgEchoLocked, GeoGem::EchoUnlockLevel);
		break;
	case EGeoGemRefusal::EchoCore:
		Message = MsgEchoCore;
		break;
	case EGeoGemRefusal::AlreadyEcho:
		Message = FText::Format(MsgAlreadyEcho, GemName);
		break;
	case EGeoGemRefusal::NoCopy:
		Message = FText::Format(MsgNoCopy, GemName);
		break;
	case EGeoGemRefusal::None:
		break;
	}

	if (Message)
	{
		UGeoMessageSubsystem::Post(this, *Message, EGeoColor::Damage);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::PostEquipEffects(UGeoGemCatalog const& Catalog, FGeoGemLoadout const& Before,
											FName const GemId) const
{
	FGeoGemInfo const* Gem = Catalog.Find(GemId);
	FText const GemName = Gem ? Gem->DisplayName.ToUpper() : FText::GetEmpty();
	if (Before.Echo == GemId)
	{
		UGeoMessageSubsystem::Post(this, FText::Format(MsgEchoCleared, GemName), EGeoColor::Neutral);
	}
	else if (Catalog.FindTier(GemId) == EGeoGemTier::Core && Before.Sockets.Contains(GemId))
	{
		UGeoMessageSubsystem::Post(this, FText::Format(MsgCoreMoved, GemName), EGeoColor::Neutral);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoGemLoadoutWidget::CountFilled(FGeoGemLoadout const& Loadout)
{
	return static_cast<int32>(Algo::CountIf(Loadout.Sockets,
											[](FName const GemId)
											{
												return !GemId.IsNone();
											}))
		+ (Loadout.Echo.IsNone() ? 0 : 1);
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
	FGeoGemLoadout const Before = Profile->GetLoadout(ShownClass);
	if (EmptyTarget && Profile->Equip(*Catalog, ShownClass, *EmptyTarget, PickedGem))
	{
		PostEquipEffects(*Catalog, Before, PickedGem);
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleFill()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	FGeoGemInfo const* Gem = Catalog ? Catalog->Find(PickedGem) : nullptr;
	int32 const Filled = Profile && Catalog ? Profile->FillSockets(*Catalog, ShownClass, PickedGem) : 0;
	if (Filled > 0 && Gem)
	{
		UGeoMessageSubsystem::Post(this, FText::Format(MsgFilled, Filled, Gem->DisplayName.ToUpper()), EGeoColor::Heal);
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleUnequipTier()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (Profile && Catalog)
	{
		int32 const FilledBefore = CountFilled(Profile->GetLoadout(ShownClass));
		for (EGeoGemTier const Tier : GetTiers())
		{
			if (TierFilter.Get(Tier) == Tier)
			{
				Profile->UnequipTier(*Catalog, ShownClass, Tier);
			}
		}

		int32 const Emptied = FilledBefore - CountFilled(Profile->GetLoadout(ShownClass));
		if (Emptied > 0)
		{
			UGeoMessageSubsystem::Post(this, FText::Format(MsgUnequipped, Emptied), EGeoColor::Heal);
		}

		ClearPick();
		CommitChanges();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoGemLoadoutWidget::HandleEcho()
{
	UGeoGemProfileSave* Profile = GetProfile();
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	FGeoGemLoadout const Before = Profile ? Profile->GetLoadout(ShownClass) : FGeoGemLoadout();
	FGeoGemInfo const* Gem = Catalog ? Catalog->Find(PickedGem) : nullptr;
	if (Profile && Catalog && Profile->SetEcho(*Catalog, ShownClass, PickedGem))
	{
		int32 const Emptied = static_cast<int32>(Algo::Count(Before.Sockets, PickedGem));
		if (Emptied > 0 && Gem)
		{
			UGeoMessageSubsystem::Post(this, FText::Format(MsgEchoEmptied, Emptied, Gem->DisplayName.ToUpper()),
									   EGeoColor::Neutral);
		}

		CommitChanges();
	}
}
