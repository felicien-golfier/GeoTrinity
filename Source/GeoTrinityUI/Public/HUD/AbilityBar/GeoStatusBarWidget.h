// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoStatusBarWidget.generated.h"

class AGeoHUD;
struct FGeoActiveEffectIcon;
class UGeoFrameStyle;
class UGeoIcon;
class UGeoMeter;
class UGeoMeterStyle;
class UTextBlock;
class UVerticalBox;

/** The widgets of one status tile, kept to update it in place. */
USTRUCT()
struct FGeoStatusTile
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGeoIcon> Icon;

	bool bDebuff = false;

	/** The boost the effect gives, under the icon. */
	UPROPERTY()
	TObjectPtr<UTextBlock> ValueText;

	/** "×2" in the top-right corner while stacked. */
	UPROPERTY()
	TObjectPtr<UTextBlock> CountText;

	/** Time left along the bottom edge. */
	UPROPERTY()
	TObjectPtr<UGeoMeter> TimeMeter;

	/** Seconds left, under the tile. */
	UPROPERTY()
	TObjectPtr<UTextBlock> TimeText;
};

/**
 * One tile per active effect on the local player that carries an icon, growing rightward, a new row starting above the
 * last once a row holds TilesPerRow: a framed square with the icon, the boost the effect gives, the stack count and a
 * depletion bar along its bottom, the seconds left under it. A debuff (an effect from a hostile actor) wears
 * DebuffTileStyle and DebuffColor. Built in C++: the tree is the rows of tiles alone, bottom-aligned in the fixed zone
 * WBP_MainOverlay gives it right of the ability bar, so the first row stays level with the abilities. Polls AGeoHUD::GetActiveEffectIcons
 * each tick; tiles are rebuilt only when the set of icons or debuffs changes, and updated in place otherwise.
 */
UCLASS()
class GEOTRINITYUI_API UGeoStatusBarWidget : public UGeoUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the HUD reference the tick polls for active effect icons. Called by AGeoHUD right after CreateWidget. */
	void InitStatusBar(AGeoHUD* GeoHUD);

protected:
	/** Constructs the widget tree: the tile row as its root. */
	virtual bool Initialize() override;
	/** Polls AGeoHUD::GetActiveEffectIcons, rebuilds the tiles when the set changes and updates them in place. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	TObjectPtr<UGeoFrameStyle> TileStyle;

	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	TObjectPtr<UGeoFrameStyle> DebuffTileStyle;

	/** The depletion bar along a tile's bottom. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	TObjectPtr<UGeoMeterStyle> TimeMeterStyle;

	/** Tints a debuff's depletion bar and time. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	FLinearColor DebuffColor = FLinearColor(1.f, .1f, .16f, 1.f);

	UPROPERTY(EditAnywhere, Category = "GeoStatusBar", meta = (ClampMin = "16", ClampMax = "256"))
	float TileSize = 56.f;

	UPROPERTY(EditAnywhere, Category = "GeoStatusBar", meta = (ClampMin = "4", ClampMax = "128"))
	float IconSize = 22.f;

	/** Space between two tiles, and between two rows. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar", meta = (ClampMin = "0", ClampMax = "64"))
	float TileGap = 6.f;

	/** Tiles in a row before the next row starts above it; keep WBP_MainOverlay's StatusZone this many tiles wide. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar", meta = (ClampMin = "1", ClampMax = "16"))
	int32 TilesPerRow = 5;

	/** Font size of the value, the count and the time, all in the theme's Mono role. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar", meta = (ClampMin = "6", ClampMax = "64"))
	int32 TextSize = 13;

	/** Space between a tile and the seconds under it. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar", meta = (ClampMin = "0", ClampMax = "32"))
	float TimeGap = 4.f;

	/** Where the stack count sits inside the tile's top-right corner. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	FMargin CountPadding = FMargin(0.f, 1.f, 3.f, 0.f);

	/** {0} is the stack count. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	FText CountFormat = INVTEXT("\u00D7{0}");

	/** Shown under a tile whose effect never runs out. */
	UPROPERTY(EditAnywhere, Category = "GeoStatusBar")
	FText InfiniteTimeText = INVTEXT("\u221E");

private:
	/** Builds the tile showing Entry and adds it to the row. */
	void AddTile(FGeoActiveEffectIcon const& Entry);
	/** A text in the theme's Mono role at TextSize. */
	UTextBlock* MakeText() const;

	UPROPERTY()
	/** The rows of tiles, the newest row on top. */
	TObjectPtr<UVerticalBox> StatusBox;

	UPROPERTY()
	TObjectPtr<AGeoHUD> HUD;

	/** Tiles shown, in the order of the HUD's entries. */
	UPROPERTY()
	TArray<FGeoStatusTile> Tiles;
};
