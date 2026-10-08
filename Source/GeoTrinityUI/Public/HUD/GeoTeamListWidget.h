// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/GeoUserWidget.h"

#include "GeoTeamListWidget.generated.h"

class AGeoPlayerState;
class UGeoPlayerCardWidget;
class UVerticalBox;

/**
 * The other players of the match, one RowClass card each, top to bottom in join order. Rebuilt only when the roster
 * changes; each row then follows its own player.
 */
UCLASS()
class GEOTRINITYUI_API UGeoTeamListWidget : public UGeoUserWidget
{
	GENERATED_BODY()

protected:
	/** Compares the match's players with the rows shown, and rebuilds them when someone joined or left. */
	virtual void NativeTick(FGeometry const& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> RowBox;

	/** One teammate: WBP_TeamRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTeamList")
	TSubclassOf<UGeoPlayerCardWidget> RowClass;

	/** Space between two rows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoTeamList", meta = (ClampMin = "0", ClampMax = "64"))
	float RowGap = 12.f;

private:
	/** Every player of the match but this widget's own, in the game state's order. */
	TArray<AGeoPlayerState*> GetTeammates() const;

	/** Players the rows show, in order. */
	TArray<TWeakObjectPtr<AGeoPlayerState>> ShownTeammates;
};
