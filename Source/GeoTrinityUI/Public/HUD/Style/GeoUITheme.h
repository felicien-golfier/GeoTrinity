// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
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
	Button
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

/**
 * The one place the menus' look is tuned: text roles, the frame a frame without a style wears, the frame code wraps
 * around a field it builds, and the styles of every themed input (UGeoEditableTextBox, UGeoComboBoxString,
 * UGeoCheckBox, UGeoSlider, UGeoProgressBar, UGeoScrollBox).
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

	UPROPERTY(EditAnywhere, Category = "GeoText")
	TMap<EGeoTextRole, FGeoTextStyle> TextStyles;

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
};
