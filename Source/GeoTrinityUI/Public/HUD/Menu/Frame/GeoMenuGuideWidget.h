// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "HUD/Interface/GeoMenuGuideGoal.h"
#include "HUD/Menu/Frame/GeoMenuGuide.h"
#include "HUD/Style/GeoUITheme.h"
#include "Styling/SlateBrush.h"

#include "GeoMenuGuideWidget.generated.h"

class APlayerController;
class SConstraintCanvas;
class SGeoGuideFrame;
class SOverlay;
class UGeoMenuPageWidget;

/**
 * The menu guide's overlay (Frame/CLAUDE.md, "Menu guide"): while a goal is set it dims the screen but a hole around
 * the next control to click, rings the hole with a pulsing frame, labels it with a short hint, and refuses every click
 * outside the hole. Native only, no Blueprint: created and layered above the pause menu by AGeoHUD. FindTarget is the
 * whole decision of where to point; the overlay only follows it, every frame, so a menu closed or a page left mid-path
 * needs no bookkeeping.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMenuGuideWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Leads the player towards InGoal; replaces any goal running. */
	void StartGuide(EGeoMenuGuideGoal InGoal);

	/** Ends the guide: nothing dims or blocks any more. */
	void StopGuide();

	/** True while a goal is set; the frame's timer sleeps otherwise. */
	bool IsGuiding() const { return Goal != EGeoMenuGuideGoal::None; }

	/** The control Controller's pause menu shows for Goal and its hint: the pause menu's top level or the page on top
	 * answers; a closed menu answers the hint naming the key that opens it, with no widget. None goal: nothing. */
	FGeoMenuGuideTarget FindTarget(APlayerController* Controller, EGeoMenuGuideGoal Goal) const;

	/** Follows the target now: rebuilds the scrim, frame and hint when the hole or the hint changed. */
	void Refresh();

protected:
	/** The guide never takes a click itself: only its scrim parts do. */
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** What dims the screen outside the hole. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide")
	FLinearColor ScrimColor = FLinearColor(0.f, 0.f, 0.f, .72f);

	/** The pulsing outline around the hole. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide")
	FLinearColor FrameColor = FLinearColor(1.f, .58f, .03f, 1.f);

	/** The outline and the hint stand this far past the control; the hole itself is exactly the control. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide", meta = (ClampMin = "0", ClampMax = "64"))
	float HolePadding = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide", meta = (ClampMin = "0.5", ClampMax = "8"))
	float FrameThickness = 2.f;

	/** How far the outer ring of the pulse swells past the hole. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide", meta = (ClampMin = "0", ClampMax = "64"))
	float PulseReach = 10.f;

	/** Seconds of one pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide", meta = (ClampMin = "0.2", ClampMax = "10"))
	float PulseSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide")
	EGeoTextRole HintTextRole = EGeoTextRole::Body;

	/** Space between the hint label and the hole. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide", meta = (ClampMin = "0", ClampMax = "128"))
	float HintGap = 12.f;

	/** Height the hint label needs over the hole; less room above and it goes below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide", meta = (ClampMin = "0", ClampMax = "256"))
	float HintRoom = 44.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide")
	FMargin HintPadding = FMargin(14.f, 8.f);

	/** The hint while the pause menu is closed: {0} is the key that opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMenuGuide")
	FText OpenMenuFormat = INVTEXT("PRESS {0} TO OPEN THE MENU");

private:
	/** Swallows the click of a scrim part. */
	FReply SwallowClick(FGeometry const& Geometry, FPointerEvent const& PointerEvent) const;

	/** Rebuilds the canvas: the scrim around Hole (when set) and the hint label. */
	void Rebuild(TOptional<FSlateRect> const& Hole, FText const& Hint);

	EGeoMenuGuideGoal Goal = EGeoMenuGuideGoal::None;

	/** The page and goal `PrepareForGuide` last ran for: it runs again only when either changes. */
	TWeakObjectPtr<UGeoMenuPageWidget> PreparedPage;
	EGeoMenuGuideGoal PreparedGoal = EGeoMenuGuideGoal::None;

	/** The goal the canvas was built for. */
	EGeoMenuGuideGoal ShownGoal = EGeoMenuGuideGoal::None;

	/** What the canvas shows now, to rebuild only on a change. */
	TOptional<FSlateRect> ShownHole;
	FVector2f ShownCanvasSize = FVector2f::ZeroVector;
	FText ShownHint;
	bool bShown = false;

	FSlateColorBrush WhiteBrush{FLinearColor::White};
	TSharedPtr<SOverlay> Root;
	TSharedPtr<SConstraintCanvas> Canvas;
	TSharedPtr<SGeoGuideFrame> Frame;
};
