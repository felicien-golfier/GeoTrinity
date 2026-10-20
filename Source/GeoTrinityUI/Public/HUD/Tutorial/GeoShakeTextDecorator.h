// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/RichTextBlockDecorator.h"
#include "CoreMinimal.h"

#include "GeoShakeTextDecorator.generated.h"

/**
 * Rich-text decorator for <Shake>...</> runs: the run is drawn in the block's default style and jitters by up to
 * Amplitude, Frequency times a second (Cat lines, AI/Tutorial/Steps.md step 2). Added to a URichTextBlock's
 * decorator classes.
 */
UCLASS()
class GEOTRINITYUI_API UGeoShakeTextDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	/** Largest offset of the run from its place, in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShake", meta = (ClampMin = "0", ClampMax = "20"))
	float Amplitude = 2.f;

	/** How fast the run moves around, in changes of direction per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoShake", meta = (ClampMin = "0.5", ClampMax = "60"))
	float Frequency = 18.f;
};
