// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"

#include "GeoIconThumbnailRenderer.generated.h"

/** Content Browser and asset-picker thumbnail of a UGeoIcon: the icon drawn as the game draws it, on a dark tile. */
UCLASS()
class UGeoIconThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()

public:
	virtual bool CanVisualizeAsset(UObject* Object) override;
	/** Draws the backdrop on the canvas, then the icon's Slate paint into the same render target. */
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget,
					  FCanvas* Canvas, bool bAdditionalViewFamily) override;
};
