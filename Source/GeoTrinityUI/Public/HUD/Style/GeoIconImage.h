// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "GeoIconImage.generated.h"

class SGeoIconImage;
class UGeoIcon;

/** Draws a UGeoIcon — an ability, a status — with Slate lines and fills, in its game colour times Tint. */
UCLASS()
class GEOTRINITYUI_API UGeoIconImage : public UWidget
{
	GENERATED_BODY()

public:
	/** Switches to InIcon; null draws nothing. */
	UFUNCTION(BlueprintCallable, Category = "GeoIcon")
	void SetIcon(UGeoIcon const* InIcon);

	UFUNCTION(BlueprintCallable, Category = "GeoIcon")
	void SetSize(float InSize);

	/** Multiplies the icon's colour: dims a cooling-down ability, greys an empty gauge. */
	UFUNCTION(BlueprintCallable, Category = "GeoIcon")
	void SetTint(FLinearColor const& InTint);

	/** When on, also multiplies the icon by the foreground colour of the button holding it, so it follows the button's hover state. */
	UFUNCTION(BlueprintCallable, Category = "GeoIcon")
	void SetTintWithForeground(bool bInTintWithForeground);

	/** Pushes Icon, Size, Tint and bTintWithForeground to the underlying SGeoIconImage. */
	virtual void SynchronizeProperties() override;
	/** Releases the SGeoIconImage Slate widget. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	TObjectPtr<UGeoIcon const> Icon;

	/** Width and height asked of the layout; the icon fits whatever box it is given. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon", meta = (ClampMin = "1", ClampMax = "1024"))
	float Size = 44.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	FLinearColor Tint = FLinearColor::White;

	/** Also multiplies the icon by the foreground colour of the button holding it, so it follows its hover state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoIcon")
	bool bTintWithForeground = false;

private:
	TSharedPtr<SGeoIconImage> MyIcon;
};
