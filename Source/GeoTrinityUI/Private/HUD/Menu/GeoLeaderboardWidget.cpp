// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoLeaderboardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Menu/GeoListFrameWidget.h"
#include "HUD/Menu/GeoListRowWidget.h"
#include "System/GeoLeaderboardSave.h"

// Share of the row each column takes, so the header and every row line up whatever the panel is wide.
static float const RankColumnWeight = 1.f;
static float const BossLeftColumnWeight = 2.f;
static float const TimeColumnWeight = 2.f;

// A row always shows this many player columns, empty where the team was short, so they line up down the list.
static int32 const PlayersPerRow = 3;
static float const PlayerColumnWeight = 3.f;
static float const ClassIconSize = 16.f;
static int32 const PlayerNameMaxChars = 14;

// A class marker is rasterized rather than authored, so no asset carries these three shapes. Twice the size it draws
// at, with a supersampled edge, keeps it smooth at any UI scale.
static int32 const IconResolution = 32;
static int32 const IconSubSamples = 4;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Leaderboard = UGeoLeaderboardSave::Load();
	BuildClassIcons();
	BuildTabs();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardWidget::BuildTabs()
{
	TabsBox->ClearChildren();
	TabRows.Empty();

	// Entries are ranked, not grouped, so the first attempt at a boss is what puts its tab in the strip.
	for (FGeoLeaderboardEntry const& Entry : Leaderboard->Entries)
	{
		if (TabRows.Contains(Entry.ArenaTag))
		{
			continue;
		}

		UGeoListRowWidget* const Tab = MakeRow();
		if (!Tab)
		{
			return;
		}
		Tab->AddTextColumn(FText::FromName(Entry.ArenaTag.GetTagLeafName()), 0.f);
		// The tab carries the boss it stands for as the payload of its click, so the row itself holds no state.
		Tab->OnClicked.AddUObject(this, &UGeoLeaderboardWidget::SelectArena, Entry.ArenaTag);
		TabsBox->AddChild(Tab);
		TabRows.Add(Entry.ArenaTag, Tab);
	}

	SelectArena(Leaderboard->Entries.IsEmpty() ? FGameplayTag() : Leaderboard->Entries[0].ArenaTag);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardWidget::SelectArena(FGameplayTag const Arena)
{
	SelectedArena = Arena;
	for (TPair<FGameplayTag, TObjectPtr<UGeoListRowWidget>> const& Tab : TabRows)
	{
		Tab.Value->SetTint(Tab.Key == SelectedArena ? EGeoListRowTint::Selected : EGeoListRowTint::Normal);
	}

	PopulateEntries();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardWidget::PopulateEntries()
{
	ListFrame->RowsBox->ClearChildren();

	UGeoListRowWidget* const Header = MakeRow();
	if (!Header)
	{
		return;
	}
	Header->SetSelectable(false);
	Header->SetTint(EGeoListRowTint::Header);
	ListFrame->RowsBox->AddChild(Header);

	if (Leaderboard->Entries.IsEmpty())
	{
		Header->AddTextColumn(FText::FromString(TEXT("No fight recorded yet")), 0.f);
		return;
	}

	Header->AddTextColumn(FText::FromString(TEXT("#")), RankColumnWeight);
	Header->AddTextColumn(FText::FromString(TEXT("Boss left")), BossLeftColumnWeight);
	Header->AddTextColumn(FText::FromString(TEXT("Time")), TimeColumnWeight);
	Header->AddTextColumn(FText::FromString(TEXT("Players")), PlayerColumnWeight);
	for (int32 Column = 1; Column < PlayersPerRow; ++Column)
	{
		Header->AddTextColumn(FText::GetEmpty(), PlayerColumnWeight);
	}

	int32 Rank = 0;
	for (FGeoLeaderboardEntry const& Entry : Leaderboard->Entries)
	{
		if (Entry.ArenaTag != SelectedArena)
		{
			continue;
		}
		++Rank;

		// Ceil so a boss left with a sliver never reads as the kill it survived.
		FString const BossLeft = Entry.BossHealthRatio > 0.f
									 ? FString::Printf(TEXT("%d%%"), FMath::CeilToInt32(Entry.BossHealthRatio * 100.f))
									 : FString(TEXT("KILLED"));

		UGeoListRowWidget* const Row = MakeRow();
		if (!Row)
		{
			return;
		}
		Row->SetSelectable(false);
		Row->SetTint(Rank % 2 == 0 ? EGeoListRowTint::Alternate : EGeoListRowTint::Normal);
		Row->AddTextColumn(FText::AsNumber(Rank), RankColumnWeight);
		Row->AddTextColumn(FText::FromString(BossLeft), BossLeftColumnWeight);
		Row->AddTextColumn(UHudFunctionLibrary::FormatDuration(Entry.DurationSeconds), TimeColumnWeight);
		for (FGeoLeaderboardPlayer const& Player : Entry.Players)
		{
			AddPlayerColumn(Row, Player);
		}
		for (int32 Missing = Entry.Players.Num(); Missing < PlayersPerRow; ++Missing)
		{
			Row->AddTextColumn(FText::GetEmpty(), PlayerColumnWeight);
		}
		ListFrame->RowsBox->AddChild(Row);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardWidget::AddPlayerColumn(UGeoListRowWidget* Row, FGeoLeaderboardPlayer const& Player)
{
	UHorizontalBox* const Column = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// A player who never picked a class has no shape to show, only their name.
	if (UTexture2D* const Icon = ClassIcons.FindRef(Player.PlayerClass))
	{
		UImage* const Shape = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Shape->SetBrushFromTexture(Icon);
		Shape->SetDesiredSizeOverride(FVector2D(ClassIconSize, ClassIconSize));
		Column->AddChildToHorizontalBox(Shape)->SetVerticalAlignment(VAlign_Center);
	}

	FString Name = Player.PlayerName;
	if (Name.Len() > PlayerNameMaxChars)
	{
		Name = Name.Left(PlayerNameMaxChars - 3) + TEXT("...");
	}
	Column->AddChildToHorizontalBox(Row->MakeColumnText(FText::FromString(Name)))->SetPadding(FMargin(4.f, 0.f));

	Row->AddColumn(Column, PlayerColumnWeight);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLeaderboardWidget::BuildClassIcons()
{
	for (EPlayerClass const Class : {EPlayerClass::Triangle, EPlayerClass::Circle, EPlayerClass::Square})
	{
		ClassIcons.Add(Class, CreateClassIcon(Class));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UTexture2D* UGeoLeaderboardWidget::CreateClassIcon(EPlayerClass const Class)
{
	FColor const Color = GetClassColor(Class).ToFColor(true);

	TArray<FColor> Texels;
	Texels.Reserve(IconResolution * IconResolution);
	for (int32 Y = 0; Y < IconResolution; ++Y)
	{
		for (int32 X = 0; X < IconResolution; ++X)
		{
			int32 Covered = 0;
			for (int32 SampleY = 0; SampleY < IconSubSamples; ++SampleY)
			{
				for (int32 SampleX = 0; SampleX < IconSubSamples; ++SampleX)
				{
					FVector2f const Point((X + (SampleX + .5f) / IconSubSamples) / IconResolution,
										  (Y + (SampleY + .5f) / IconSubSamples) / IconResolution);
					if (IsInsideClassShape(Class, Point))
					{
						++Covered;
					}
				}
			}
			Texels.Add(FColor(Color.R, Color.G, Color.B,
							  static_cast<uint8>(Covered * 255 / (IconSubSamples * IconSubSamples))));
		}
	}

	return UTexture2D::CreateTransient(IconResolution, IconResolution, PF_B8G8R8A8, NAME_None,
									   TConstArrayView64<uint8>(reinterpret_cast<uint8 const*>(Texels.GetData()),
																static_cast<int64>(Texels.Num()) * sizeof(FColor)));
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoLeaderboardWidget::IsInsideClassShape(EPlayerClass const Class, FVector2f const Point)
{
	switch (Class)
	{
	case EPlayerClass::Triangle:
		// Apex at the top of the square, base along its bottom edge.
		return FMath::Abs(Point.X - .5f) <= .5f * Point.Y;
	case EPlayerClass::Circle:
		return (Point - FVector2f(.5f, .5f)).Size() <= .5f;
	case EPlayerClass::Square:
		// Inset, so the one shape filling its whole square does not read as the biggest of the three.
		return FMath::Max(FMath::Abs(Point.X - .5f), FMath::Abs(Point.Y - .5f)) <= .42f;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FLinearColor UGeoLeaderboardWidget::GetClassColor(EPlayerClass const Class)
{
	switch (Class)
	{
	case EPlayerClass::Triangle:
		return FLinearColor(1.f, .8f, .1f);
	case EPlayerClass::Circle:
		return FLinearColor(.2f, .9f, .4f);
	case EPlayerClass::Square:
		return FLinearColor(.2f, .5f, 1.f);
	default:
		return FLinearColor::White;
	}
}
