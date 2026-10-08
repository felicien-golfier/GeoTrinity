// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoStatsPanelWidget.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "Actor/Arena/GeoArena.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EnhancedInputSubsystems.h"
#include "GameClasses/GeoPlayerController.h"
#include "GameClasses/GeoPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Style/GeoIconImage.h"
#include "HUD/Style/GeoShape.h"
#include "HUD/Style/GeoUITheme.h"
#include "InputAction.h"
#include "Settings/GeoGameUserSettings.h"
#include "Tool/GeoIcon.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoStatsPanelWidget::UGeoStatsPanelWidget(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	ShortColumns = {EGeoStatColumn::DamageNow, EGeoStatColumn::DamageAverage, EGeoStatColumn::HealingNow,
					EGeoStatColumn::HealingAverage, EGeoStatColumn::DamageTaken};
	FullColumns = {EGeoStatColumn::DamageNow,	   EGeoStatColumn::DamageAverage,	EGeoStatColumn::DamagePeak,
				   EGeoStatColumn::DamageTotal,	   EGeoStatColumn::HealingNow,		EGeoStatColumn::HealingAverage,
				   EGeoStatColumn::HealingPeak,	   EGeoStatColumn::HealingTotal,	EGeoStatColumn::DamageTaken,
				   EGeoStatColumn::DamageModifier, EGeoStatColumn::HealingModifier, EGeoStatColumn::Armor,
				   EGeoStatColumn::Speed};
	ColumnLabels = {{EGeoStatColumn::DamageNow, INVTEXT("NOW")},
					{EGeoStatColumn::DamageAverage, INVTEXT("\u00F8")},
					{EGeoStatColumn::DamagePeak, INVTEXT("PEAK")},
					{EGeoStatColumn::DamageTotal, INVTEXT("SUM")},
					{EGeoStatColumn::HealingNow, INVTEXT("NOW")},
					{EGeoStatColumn::HealingAverage, INVTEXT("\u00F8")},
					{EGeoStatColumn::HealingPeak, INVTEXT("PEAK")},
					{EGeoStatColumn::HealingTotal, INVTEXT("SUM")},
					{EGeoStatColumn::DamageTaken, INVTEXT("SUM")},
					{EGeoStatColumn::DamageModifier, INVTEXT("DMG")},
					{EGeoStatColumn::HealingModifier, INVTEXT("HEAL")},
					{EGeoStatColumn::Armor, INVTEXT("ARMR")},
					{EGeoStatColumn::Speed, INVTEXT("SPD")}};
	GroupLabels = {{EGeoStatGroup::Damage, INVTEXT("DAMAGE / S")},
				   {EGeoStatGroup::Healing, INVTEXT("HEALING / S")},
				   {EGeoStatGroup::DamageTaken, INVTEXT("TAKEN")},
				   {EGeoStatGroup::Modifiers, INVTEXT("MODIFIERS")}};
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGeoGameUserSettings* Settings = UGeoGameUserSettings::Get();
	ShowPanel(Settings->ShowCombatStats());
	Settings->OnShowCombatStatsChanged.AddUObject(this, &UGeoStatsPanelWidget::ShowPanel);
	if (AGeoPlayerController* Controller = GetOwningPlayer<AGeoPlayerController>())
	{
		Controller->OnToggleStatsDetail.AddUObject(this, &UGeoStatsPanelWidget::ToggleDetail);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::NativeDestruct()
{
	UGeoGameUserSettings::Get()->OnShowCombatStatsChanged.RemoveAll(this);
	if (AGeoPlayerController* Controller = GetOwningPlayer<AGeoPlayerController>())
	{
		Controller->OnToggleStatsDetail.RemoveAll(this);
	}
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::ShowPanel(bool const bShow)
{
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::ToggleDetail()
{
	bFullTable = !bFullTable;
	TablePlayers.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<EGeoStatColumn> const& UGeoStatsPanelWidget::GetShownColumns() const
{
	return bFullTable ? FullColumns : ShortColumns;
}

// ---------------------------------------------------------------------------------------------------------------------
EGeoStatGroup UGeoStatsPanelWidget::GetGroup(EGeoStatColumn const Column)
{
	switch (Column)
	{
	case EGeoStatColumn::DamageNow:
	case EGeoStatColumn::DamageAverage:
	case EGeoStatColumn::DamagePeak:
	case EGeoStatColumn::DamageTotal:
		return EGeoStatGroup::Damage;
	case EGeoStatColumn::HealingNow:
	case EGeoStatColumn::HealingAverage:
	case EGeoStatColumn::HealingPeak:
	case EGeoStatColumn::HealingTotal:
		return EGeoStatGroup::Healing;
	case EGeoStatColumn::DamageTaken:
		return EGeoStatGroup::DamageTaken;
	default:
		return EGeoStatGroup::Modifiers;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<AGeoPlayerState*> UGeoStatsPanelWidget::GetPlayers() const
{
	TArray<AGeoPlayerState*> Players;
	AGeoPlayerState* OwningPlayer = GetOwningPlayerState<AGeoPlayerState>();
	if (OwningPlayer)
	{
		Players.Add(OwningPlayer);
	}

	AGameStateBase const* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	for (APlayerState* PlayerState : GameState ? GameState->PlayerArray : TArray<TObjectPtr<APlayerState>>())
	{
		AGeoPlayerState* Player = Cast<AGeoPlayerState>(PlayerState);
		if (Player && Player != OwningPlayer)
		{
			Players.Add(Player);
		}
	}
	return Players;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::NativeTick(FGeometry const& MyGeometry, float const InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TArray<AGeoPlayerState*> const Players = GetPlayers();
	bool bPlayersChanged = Players.Num() != TablePlayers.Num();
	for (int32 Index = 0; Index < Players.Num() && !bPlayersChanged; ++Index)
	{
		bPlayersChanged = TablePlayers[Index].Key != Players[Index]
					   || TablePlayers[Index].Value != static_cast<uint8>(Players[Index]->GetPlayerClass());
	}
	if (bPlayersChanged)
	{
		BuildTable(Players);
	}

	if (AGeoArena* FightingArena = AGeoArena::GetFightingArena(this))
	{
		Arena = FightingArena;
	}
	if (AGeoArena const* FoughtArena = Arena.Get(); FoughtArena && FightTimeText)
	{
		FightTimeText->SetText(UHudFunctionLibrary::FormatDuration(FoughtArena->GetFightElapsedSeconds()));
	}
	for (FGeoStatsRow const& Row : Rows)
	{
		WriteRow(Row, GetShownColumns());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UTextBlock* UGeoStatsPanelWidget::MakeText(FText const& Text, int32 const Size, FLinearColor const& Color) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UGeoUITheme::ApplyTextStyle(TextBlock, EGeoTextRole::Mono);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = Size;
	TextBlock->SetFont(Font);
	TextBlock->SetColorAndOpacity(Color);
	TextBlock->SetShadowColorAndOpacity(ShadowColor);
	TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
	TextBlock->SetText(Text);
	return TextBlock;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::AddCell(UWidget* Content, int32 const Row, int32 const Column, int32 const ColumnSpan,
								   EHorizontalAlignment const Alignment) const
{
	UGridSlot* const CellSlot = StatsGrid->AddChildToGrid(Content, Row, Column);
	CellSlot->SetColumnSpan(ColumnSpan);
	CellSlot->SetHorizontalAlignment(Alignment);
	CellSlot->SetVerticalAlignment(VAlign_Center);
	CellSlot->SetPadding(FMargin(0.f, 0.f, ColumnGap, RowGap));
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoStatsPanelWidget::BuildHeadings(TArray<EGeoStatColumn> const& Columns)
{
	UHorizontalBox* Time = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UGeoIconImage* Clock = WidgetTree->ConstructWidget<UGeoIconImage>(UGeoIconImage::StaticClass());
	Clock->SetIcon(FightTimeIcon);
	Clock->SetSize(IconSize);
	Time->AddChildToHorizontalBox(Clock)->SetVerticalAlignment(VAlign_Center);
	FightTimeText = MakeText(FText::FromString(TEXT("0:00")), SecondarySize, SecondaryColor);
	UHorizontalBoxSlot* const TimeSlot = Time->AddChildToHorizontalBox(FightTimeText);
	TimeSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
	TimeSlot->SetVerticalAlignment(VAlign_Center);
	AddCell(Time, 0, 0, 2, HAlign_Left);

	for (int32 First = 0; First < Columns.Num();)
	{
		EGeoStatGroup const Group = GetGroup(Columns[First]);
		int32 Span = 1;
		while (First + Span < Columns.Num() && GetGroup(Columns[First + Span]) == Group)
		{
			++Span;
		}

		UVerticalBox* Heading = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UWidget* Title = nullptr;
		if (UGeoIcon const* Icon = GroupIcons.FindRef(Group))
		{
			UGeoIconImage* IconImage = WidgetTree->ConstructWidget<UGeoIconImage>(UGeoIconImage::StaticClass());
			IconImage->SetIcon(Icon);
			IconImage->SetSize(IconSize);
			IconImage->SetToolTipText(GroupLabels.FindRef(Group));
			Title = IconImage;
		}
		else
		{
			Title = MakeText(GroupLabels.FindRef(Group), LabelSize, SecondaryColor);
		}
		Heading->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
		USizeBox* RuleHeight = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		RuleHeight->SetHeightOverride(1.f);
		UBorder* Rule = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Rule->SetBrushColor(RuleColor);
		RuleHeight->SetContent(Rule);
		UVerticalBoxSlot* const RuleSlot = Heading->AddChildToVerticalBox(RuleHeight);
		RuleSlot->SetHorizontalAlignment(HAlign_Fill);
		RuleSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		AddCell(Heading, 0, 2 + First, Span, HAlign_Fill);
		First += Span;
	}

	for (int32 Index = 0; bFullTable && Index < Columns.Num(); ++Index)
	{
		AddCell(MakeText(ColumnLabels.FindRef(Columns[Index]), LabelSize, SecondaryColor), 1, 2 + Index);
	}
	return bFullTable ? 2 : 1;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::BuildTable(TArray<AGeoPlayerState*> const& Players)
{
	StatsGrid->ClearChildren();
	Rows.Reset();
	TablePlayers.Reset();

	TArray<EGeoStatColumn> const& Columns = GetShownColumns();
	int32 const FirstPlayerRow = BuildHeadings(Columns);
	UGeoUITheme const* Theme = UGeoUITheme::Get();
	for (int32 PlayerIndex = 0; PlayerIndex < Players.Num(); ++PlayerIndex)
	{
		AGeoPlayerState* Player = Players[PlayerIndex];
		TablePlayers.Emplace(Player, static_cast<uint8>(Player->GetPlayerClass()));
		bool const bOwningPlayer = PlayerIndex == 0 && Player == GetOwningPlayerState();
		float const Opacity = bOwningPlayer ? 1.f : OtherPlayerOpacity;
		int32 const GridRow = FirstPlayerRow + PlayerIndex;

		UGeoShape* Shape = WidgetTree->ConstructWidget<UGeoShape>(UGeoShape::StaticClass());
		Shape->Size = ShapeSize;
		if (FGeoClassStyle const* ClassStyle = Theme ? Theme->FindClassStyle(Player->GetPlayerClass()) : nullptr)
		{
			Shape->SetClassShape(*ClassStyle);
		}
		Shape->SetRenderOpacity(Opacity);
		AddCell(Shape, GridRow, 0, 1, HAlign_Center);

		FString const Name = Player->GetPlayerName().Left(NameLength).ToUpper();
		UTextBlock* NameText =
			MakeText(FText::FromString(Name), NameSize, bOwningPlayer ? ValueColor : NameColor);
		NameText->SetToolTipText(FText::FromString(Player->GetPlayerName()));
		NameText->SetRenderOpacity(Opacity);
		AddCell(NameText, GridRow, 1, 1, HAlign_Left);

		FGeoStatsRow& Row = Rows.AddDefaulted_GetRef();
		Row.Player = Player;
		for (int32 Index = 0; Index < Columns.Num(); ++Index)
		{
			bool const bPrimary = Columns[Index] == EGeoStatColumn::DamageNow
							   || Columns[Index] == EGeoStatColumn::HealingNow
							   || Columns[Index] == EGeoStatColumn::DamageTaken;
			UTextBlock* Cell = MakeText(FText::GetEmpty(), bPrimary ? ValueSize : SecondarySize, ValueColor);
			Cell->SetRenderOpacity(Opacity);
			AddCell(Cell, GridRow, 2 + Index);
			Row.Cells.Add(Cell);
		}
	}

	if (ToggleHintText)
	{
		FText const Key = GetToggleKeyName();
		FSlateFontInfo Font = ToggleHintText->GetFont();
		Font.Size = LabelSize;
		ToggleHintText->SetFont(Font);
		ToggleHintText->SetColorAndOpacity(ZeroColor);
		ToggleHintText->SetShadowColorAndOpacity(ShadowColor);
		ToggleHintText->SetShadowOffset(FVector2D(1.f, 1.f));
		ToggleHintText->SetVisibility(Key.IsEmpty() ? ESlateVisibility::Collapsed
													: ESlateVisibility::HitTestInvisible);
		ToggleHintText->SetText(FText::Format(bFullTable ? LessFormat : MoreFormat, Key));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoStatsPanelWidget::WriteRow(FGeoStatsRow const& Row, TArray<EGeoStatColumn> const& Columns) const
{
	AGeoPlayerState const* Player = Row.Player.Get();
	UCharacterAttributeSet const* Attributes = Player ? Player->GetCharacterAttributeSet() : nullptr;
	for (int32 Index = 0; Player && Index < Columns.Num() && Index < Row.Cells.Num(); ++Index)
	{
		float Value = 0.f;
		float Neutral = 0.f;
		bool bPrimary = false;
		FText Text;
		auto Figure = [&Value, &Text](float const InValue)
		{
			Value = InValue;
			Text = UHudFunctionLibrary::FormatCompactNumber(InValue);
		};
		auto Multiplier = [&Value, &Neutral, &Text](float const InValue)
		{
			Value = InValue;
			Neutral = 1.f;
			Text = FText::FromString(FString::Printf(TEXT("\u00D7%.2f"), InValue));
		};
		switch (Columns[Index])
		{
		case EGeoStatColumn::DamageNow:
			Figure(Player->GetLiveDPS());
			bPrimary = true;
			break;
		case EGeoStatColumn::DamageAverage:
			Figure(Player->GetFightDPS());
			Text = FText::Format(AverageFormat, Text);
			break;
		case EGeoStatColumn::DamagePeak:
			Figure(Player->GetMaxBurstDamage());
			break;
		case EGeoStatColumn::DamageTotal:
			Figure(Player->GetTotalDamageDealt());
			break;
		case EGeoStatColumn::HealingNow:
			Figure(Player->GetLiveHPS());
			bPrimary = true;
			break;
		case EGeoStatColumn::HealingAverage:
			Figure(Player->GetFightHPS());
			Text = FText::Format(AverageFormat, Text);
			break;
		case EGeoStatColumn::HealingPeak:
			Figure(Player->GetMaxBurstHealing());
			break;
		case EGeoStatColumn::HealingTotal:
			Figure(Player->GetTotalHealingDealt());
			break;
		case EGeoStatColumn::DamageTaken:
			Figure(Player->GetTotalDamageReceived());
			bPrimary = true;
			break;
		case EGeoStatColumn::DamageModifier:
			Multiplier(Attributes ? Attributes->GetDamageMultiplier() : 1.f);
			break;
		case EGeoStatColumn::HealingModifier:
			Value = Attributes ? Attributes->GetAppliedHealBoost() : 1.f;
			Neutral = 1.f;
			Text = FText::FromString(FString::Printf(TEXT("%+d%%"), FMath::RoundToInt((Value - 1.f) * 100.f)));
			break;
		case EGeoStatColumn::Armor:
			Value = Attributes ? Attributes->GetDamageReduction() : 0.f;
			Text = FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)));
			break;
		case EGeoStatColumn::Speed:
			Multiplier(Attributes ? Attributes->GetMovementSpeedMultiplier() : 1.f);
			break;
		}

		UTextBlock* Cell = Row.Cells[Index];
		Cell->SetText(Text);
		Cell->SetColorAndOpacity(FMath::IsNearlyEqual(Value, Neutral, .005f) ? ZeroColor
								 : bPrimary									 ? ValueColor
																			 : SecondaryColor);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FText UGeoStatsPanelWidget::GetToggleKeyName() const
{
	AGeoPlayerController const* Controller = GetOwningPlayer<AGeoPlayerController>();
	UEnhancedInputLocalPlayerSubsystem const* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	UInputAction const* Action = Controller ? Controller->ToggleStatsDetailAction.LoadSynchronous() : nullptr;
	TArray<FKey> const Keys =
		Action && InputSubsystem ? InputSubsystem->QueryKeysMappedToAction(Action) : TArray<FKey>();
	return Keys.Num() > 0 ? Keys[0].GetDisplayName(false) : FText::GetEmpty();
}
