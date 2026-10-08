// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Border.h"
#include "CoreMinimal.h"

#include "GeoFrame.generated.h"

class SGeoFrame;
class UGeoFrameStyle;

/**
 * A Border that draws a UGeoFrameStyle around its content: outline, glow, fill, grid and the runners travelling the
 * outline. Wraps anything — a button, a panel, a list row, a text field, the whole screen. Idle unless hovered or
 * holding focus (with bActivateOnHoverAndFocus) or set active (a selected tab or row), and eases between the two.
 * Draws with Slate primitives on an active timer: no material, texture or UserWidget tick.
 */
UCLASS()
class GEOTRINITYUI_API UGeoFrame : public UBorder
{
	GENERATED_BODY()

public:
	/** Clears the Border's own background, so only the frame style draws. */
	UGeoFrame(FObjectInitializer const& ObjectInitializer);

	/** Shows the active look whatever the hover and focus, for a selected tab or row. */
	UFUNCTION(BlueprintCallable, Category = "GeoFrame")
	void SetActive(bool bInActive);

	/** Wears InFrameStyle; null falls back to the theme's default frame. */
	UFUNCTION(BlueprintCallable, Category = "GeoFrame")
	void SetFrameStyle(UGeoFrameStyle* InFrameStyle);

	/** Turns hover and focus activation on or off, for a row that is only read rather than clicked. */
	void SetActivateOnHoverAndFocus(bool bInActivate);

	/** Multiplies the style's glow colours, so one style glows in each class's colour. */
	UFUNCTION(BlueprintCallable, Category = "GeoFrame")
	void SetGlowTint(FLinearColor const& InGlowTint);

	/** Multiplies the style's line and runner colours, so one style draws its outline in each class's colour. */
	UFUNCTION(BlueprintCallable, Category = "GeoFrame")
	void SetLineTint(FLinearColor const& InLineTint);
	
	/** Pushes FrameStyle, bActive, bActivateOnHoverAndFocus and the tints to the underlying SGeoFrame. */
	virtual void SynchronizeProperties() override;
	/** Releases the SGeoFrame Slate widget. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

protected:
	/** Builds an SGeoFrame in place of the plain SBorder. */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Look of the frame. Empty wears the theme's DefaultFrameStyle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame")
	TObjectPtr<UGeoFrameStyle> FrameStyle;

	/** Goes active while the mouse is over it or keyboard/gamepad focus is inside it — on for anything clickable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame")
	bool bActivateOnHoverAndFocus = false;

	/** Active whatever the hover and focus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame")
	bool bActive = false;

	/** Multiplies the style's glow colours; white keeps the style's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame")
	FLinearColor GlowTint = FLinearColor::White;

	/** Multiplies the style's line and runner colours; white keeps the style's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoFrame")
	FLinearColor LineTint = FLinearColor::White;

private:
	TSharedPtr<SGeoFrame> MyFrame;
};
