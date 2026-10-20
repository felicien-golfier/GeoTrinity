// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"

#include "GeoCatFaceWidget.generated.h"

class SGeoCatFace;

/** The Cat's animated face (SGeoCatFace) in the bosses' colour shades. The mouth moves while it talks. */
UCLASS()
class GEOTRINITYUI_API UGeoCatFaceWidget : public UWidget
{
	GENERATED_BODY()

public:
	/** The mouth moves while true, stays closed otherwise. */
	UFUNCTION(BlueprintCallable, Category = "GeoCatFace")
	void SetTalking(bool bInTalking);

	/** Pushes the palette, size and talking flag to the underlying SGeoCatFace. */
	virtual void SynchronizeProperties() override;
	/** Releases the SGeoCatFace Slate widget. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Head, ears and the panel border: the boss colour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatFace")
	FLinearColor Base = FLinearColor(1.f, .102f, .047f);

	/** Inner ears: a darker shade of Base. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatFace")
	FLinearColor Shade = FLinearColor(.55f, .056f, .026f);

	/** Mouth: the darkest shade of Base. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatFace")
	FLinearColor Ink = FLinearColor(.12f, .012f, .006f);

	/** Eyes and nose: a pale tint of Base, drawn with a soft halo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatFace")
	FLinearColor Light = FLinearColor(1.f, .87f, .84f);

	/** Width and height asked of the layout; the face fits whatever box it is given. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatFace", meta = (ClampMin = "1", ClampMax = "1024"))
	float Size = 56.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatFace")
	bool bTalking = false;

private:
	TSharedPtr<SGeoCatFace> MyFace;
};
