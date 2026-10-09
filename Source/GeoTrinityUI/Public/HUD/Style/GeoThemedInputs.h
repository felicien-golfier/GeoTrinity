// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "CoreMinimal.h"
#include "HUD/Style/GeoUITheme.h"

#include "GeoThemedInputs.generated.h"

// The engine inputs, each wearing its style from the UI theme instead of one authored on the widget, so every field of
// a kind looks the same and is tuned in one place. Drop-in replacements: a BindWidget of the engine type accepts them.

/** EditableTextBox wearing the theme's EditableTextBoxStyle. */
UCLASS()
class GEOTRINITYUI_API UGeoEditableTextBox : public UEditableTextBox
{
	GENERATED_BODY()

public:
	/** Applies the theme's EditableTextBoxStyle, with this field's own padding when it overrides it. */
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

	/** Gives this field its own padding, for one sitting inside something smaller than a form row. */
	void SetPaddingOverride(FMargin const& InPadding);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoInput", meta = (InlineEditConditionToggle))
	bool bOverridePadding = false;

	/** Space between the field's edge and its text, in place of the theme's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoInput", meta = (EditCondition = "bOverridePadding"))
	FMargin PaddingOverride;
};

/** ComboBoxString wearing the theme's ComboBoxStyle and ComboBoxItemStyle, its text in TextRole. */
UCLASS()
class GEOTRINITYUI_API UGeoComboBoxString : public UComboBoxString
{
	GENERATED_BODY()

public:
	/** Applies the theme's ComboBoxStyle and ComboBoxItemStyle. */
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoText")
	EGeoTextRole TextRole = EGeoTextRole::Body;

protected:
	/** Takes the font and colour of TextRole, which the engine only reads when it builds the widget. */
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

/** CheckBox wearing the theme's CheckBoxStyle. */
UCLASS()
class GEOTRINITYUI_API UGeoCheckBox : public UCheckBox
{
	GENERATED_BODY()

public:
	/** Applies the theme's CheckBoxStyle. */
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif
};

/** Slider wearing the theme's SliderStyle. */
UCLASS()
class GEOTRINITYUI_API UGeoSlider : public USlider
{
	GENERATED_BODY()

public:
	/** Applies the theme's SliderStyle. */
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif
};

/** ProgressBar wearing the theme's ProgressBarStyle. */
UCLASS()
class GEOTRINITYUI_API UGeoProgressBar : public UProgressBar
{
	GENERATED_BODY()

public:
	/** Applies the theme's ProgressBarStyle. */
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif
};

/** ScrollBox whose scroll bar wears the theme's ScrollBarStyle. */
UCLASS()
class GEOTRINITYUI_API UGeoScrollBox : public UScrollBox
{
	GENERATED_BODY()

public:
	/** Applies the theme's ScrollBarStyle. */
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif
};
