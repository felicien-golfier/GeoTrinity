// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "HUD/Menu/Frame/GeoMessageSubsystem.h"
#include "HUD/Style/GeoUITheme.h"

#include "GeoMessageLog.generated.h"

class SGeoMessageLog;

/**
 * The message strip in the bottom row of the menu window: paints the live messages of UGeoMessageSubsystem, bottom
 * line first, one clipped line each, in a theme text role and the message's colour. Keeps no message of its own, so
 * every log shows the same state. Slate-drawn on an active timer that sleeps while no message lives, and never takes
 * input.
 */
UCLASS()
class GEOTRINITYUI_API UGeoMessageLog : public UWidget
{
	GENERATED_BODY()

public:
	/** The subsystem of this widget's game instance; null in the designer. */
	UGeoMessageSubsystem* GetMessages() const;

	/** MaxLines, Lifetime, FadeTime and SlideTime as the subsystem lays messages out with. */
	FGeoMessageLayoutParams GetLayoutParams() const;

	/** Releases the SGeoMessageLog Slate widget and stops listening for posts. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

	/** Lines shown at most; a further message expires the oldest at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages", meta = (ClampMin = "1", ClampMax = "12"))
	int32 MaxLines = 5;

	/** Height of one line; MaxLines times it is the strip's height, about BACK's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages", meta = (ClampMin = "1"))
	float LineHeight = 20.f;

	/** Space left of the text: BACK's width, so the strip sits in the same place whether BACK shows or not. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages", meta = (ClampMin = "0"))
	float LeftInset = 0.f;

	/** Seconds a message is shown before it fades out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages", meta = (ClampMin = "0"))
	float Lifetime = 4.f;

	/** Seconds of a fade in or out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages", meta = (ClampMin = "0"))
	float FadeTime = .4f;

	/** Seconds a line takes to ease into a freed slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages", meta = (ClampMin = "0"))
	float SlideTime = .25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoMessages")
	EGeoTextRole TextRole = EGeoTextRole::Label;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGeoMessageLog> MyLog;
	FDelegateHandle PostedHandle;
};
