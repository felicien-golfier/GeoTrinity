// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "Gem/GeoGemTypes.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"

#include "GeoUITheme.generated.h"

class UGeoFrameStyle;
class UTextBlock;

/** What a piece of text is for; the theme gives each role its font, colour and case. */
UENUM(BlueprintType)
enum class EGeoTextRole : uint8
{
	/** Keeps the font and colour authored on the widget; the theme leaves it alone. */
	Custom,
	Title,
	Heading,
	Body,
	/** Small caption over a field or a column. */
	Label,
	/** Numbers and codes: times, pings, IPs, versions. */
	Mono,
	Button,
	/** Drawn over graphics such as a bar: its font's outline keeps it legible over a full or an empty fill. */
	Overlay,
	/** A cell of a dense table, such as the gem totals: smaller than Body, so long names fit their column. */
	Table
};

/** The look of one text role. */
USTRUCT(BlueprintType)
struct FGeoTextStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoText")
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoText")
	FSlateColor Color = FSlateColor(FLinearColor::White);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoText")
	ETextTransformPolicy Transform = ETextTransformPolicy::None;
};

/** How the UI shows a player class: its name, its role, its shape and its colour. */
USTRUCT(BlueprintType)
struct FGeoClassStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoClass")
	FText Name;

	/** What the class does in the team: DPS, HEAL, TANK. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoClass")
	FText Role;

	/** Corner count of the class shape, as a UGeoShape draws it: below 3 is a circle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoClass", meta = (ClampMin = "0", ClampMax = "12"))
	int32 Sides = 0;

	/** Turn of the class shape in degrees; 45 sets a square flat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoClass")
	float Rotation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoClass")
	FLinearColor Color = FLinearColor::White;
};

/** The polygon a gem tier is drawn as. */
USTRUCT(BlueprintType)
struct FGeoGemTierShape
{
	GENERATED_BODY()

	/** Corner count: 4 a diamond, 5 a pentagon, 6 a hexagon, 8 an octagon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "3", ClampMax = "12"))
	int32 Sides = 4;

	/** Turn in degrees, clockwise from a corner straight up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	float Rotation = 0.f;
};

/** How a gem and the socket it sits in are drawn (UGeoGemGlyph): the shape tells the tier, the colour the stat. */
USTRUCT(BlueprintType)
struct FGeoGemStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TMap<EGeoGemTier, FGeoGemTierShape> TierShapes = {{EGeoGemTier::Chip, {4, 0.f}},
													   {EGeoGemTier::Cut, {6, 0.f}},
													   {EGeoGemTier::Prism, {5, 0.f}},
													   {EGeoGemTier::Core, {8, 22.5f}}};

	/** Size of the bright facet on a gem, as a share of the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0", ClampMax = "1"))
	float FacetScale = .45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor FacetColor = FLinearColor(1.f, 1.f, 1.f, .38f);

	/** A Core has a bright centre in place of the facet, inside a dark ring that sets it apart from the other tiers;
	 * both sized as a share of the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0", ClampMax = "1"))
	float CoreRingScale = .63f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor CoreRingColor = FLinearColor(0.f, 0.f, 0.f, .55f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0", ClampMax = "1"))
	float CoreCentreScale = .32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor CoreCentreColor = FLinearColor(1.f, 1.f, 1.f, .9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor SocketFillColor = FLinearColor(.0012f, .0012f, .003f, 1.f);

	/** Line of an open socket. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor SocketLineColor = FLinearColor(.26f, .2f, .43f, 1.f);

	/** Line of a socket the class level has not opened yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor LockedLineColor = FLinearColor(.042f, .032f, .085f, 1.f);

	/** The level written in a locked socket. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor LockTextColor = FLinearColor(.19f, .14f, .34f, 1.f);

	/** Line and glow of a highlighted glyph: the selected socket, or one the selected gem can go in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor HighlightColor = FLinearColor(.9f, .85f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0.5", ClampMax = "8"))
	float LineThickness = 1.5f;

	/** Font size of a locked socket's unlock level, as a share of the socket. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0.1", ClampMax = "1"))
	float LockTextScale = .36f;
};

/**
 * The one place the menus' look is tuned: text roles, class and gem looks, the frame a frame without a style wears,
 * the frame code wraps around a field it builds, and the styles of every themed input (UGeoEditableTextBox,
 * UGeoComboBoxString, UGeoCheckBox, UGeoSlider, UGeoProgressBar, UGeoScrollBox).
 * Named in Project Settings > Geo UI; every themed widget reads it, so editing it re-skins all of them.
 */
UCLASS(BlueprintType)
class GEOTRINITYUI_API UGeoUITheme : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The project's theme, loaded and kept resident. Null, with an ensure, when Project Settings names none. */
	static UGeoUITheme const* Get();

	/** Gives Text the font, colour and case of Role. Custom leaves it as authored. */
	static void ApplyTextStyle(UTextBlock* Text, EGeoTextRole Role);

	/** The style of Role, or null with an ensure when the theme does not define it. */
	FGeoTextStyle const* FindTextStyle(EGeoTextRole Role) const;

	/** The style of PlayerClass, or null with an ensure when the theme does not define it. */
	FGeoClassStyle const* FindClassStyle(EPlayerClass PlayerClass) const;

	UPROPERTY(EditAnywhere, Category = "GeoText")
	TMap<EGeoTextRole, FGeoTextStyle> TextStyles;

	UPROPERTY(EditAnywhere, Category = "GeoClass")
	TMap<EPlayerClass, FGeoClassStyle> ClassStyles;

	UPROPERTY(EditAnywhere, Category = "GeoGem")
	FGeoGemStyle GemStyle;

	/** Worn by every UGeoFrame that names no style of its own. */
	UPROPERTY(EditAnywhere, Category = "GeoFrame")
	TObjectPtr<UGeoFrameStyle> DefaultFrameStyle;

	/** Worn by the frame code puts around a field it builds itself, such as a key-binding cell. */
	UPROPERTY(EditAnywhere, Category = "GeoFrame")
	TObjectPtr<UGeoFrameStyle> FieldFrameStyle;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FEditableTextBoxStyle EditableTextBoxStyle;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FComboBoxStyle ComboBoxStyle;

	/** A line of an open combo box's list. */
	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FTableRowStyle ComboBoxItemStyle;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FCheckBoxStyle CheckBoxStyle;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FSliderStyle SliderStyle;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FProgressBarStyle ProgressBarStyle;

	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FScrollBarStyle ScrollBarStyle;

	/** A small icon button set on something else, a tab's rename and remove: its foreground tints a UGeoIconImage
	 * following it, dim at rest and lit on hover. */
	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FButtonStyle IconButtonStyle;

	/** An icon button whose icon draws its own outline, the dashed add tab: only its background answers hover. */
	UPROPERTY(EditAnywhere, Category = "GeoInput")
	FButtonStyle OutlinedIconButtonStyle;

	/** Space between neighbouring menu buttons of one column or row; the widget builders lay every button stack out
	 * with it. */
	UPROPERTY(EditAnywhere, Category = "GeoLayout", meta = (ClampMin = "0", ClampMax = "64"))
	float MenuButtonGap = 10.f;

	/** Space between the screen's edges and the window every menu page wears (UGeoMenuFrameWidget), whatever the page
	 * holds. */
	UPROPERTY(EditAnywhere, Category = "GeoLayout")
	FMargin MenuFrameMargin = FMargin(64.f, 48.f);
};
