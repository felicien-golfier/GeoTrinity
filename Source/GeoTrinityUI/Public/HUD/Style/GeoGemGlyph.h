// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "Gem/GeoGemTypes.h"

#include "GeoGemGlyph.generated.h"

class SGeoGemGlyph;

/** What a UGeoGemGlyph shows. */
UENUM(BlueprintType)
enum class EGeoGemGlyphState : uint8
{
	/** A gem, solid in its colour with a bright facet. */
	Gem,
	/** An open socket waiting for a gem of its tier. */
	EmptySocket,
	/** A socket the class level has not opened yet, showing the level that opens it. */
	LockedSocket
};

/**
 * A gem, or the socket one sits in, drawn with Slate primitives: the tier's polygon from the theme's GemStyle, a gem in
 * its stat colour, a socket dark with a line. Highlighted, it wears the style's highlight line and glow.
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemGlyph : public UWidget
{
	GENERATED_BODY()

public:
	void SetGem(EGeoGemTier InTier, FLinearColor const& InColor);

	/** An open socket, or a locked one showing InUnlockLevel. */
	void SetSocket(EGeoGemTier InTier, bool bLocked, int32 InUnlockLevel);

	void SetHighlighted(bool bInHighlighted);

	void SetSize(float InSize);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual FText const GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	EGeoGemTier Tier = EGeoGemTier::Chip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	EGeoGemGlyphState State = EGeoGemGlyphState::Gem;

	/** A gem's colour; a socket takes its colours from the theme. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor Color = FLinearColor::White;

	/** Shown on a locked socket. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "1", ClampMax = "99"))
	int32 UnlockLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	bool bHighlighted = false;

	/** Width and height asked of the layout; the glyph fits whatever box it is given. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "4", ClampMax = "512"))
	float Size = 40.f;

private:
	TSharedPtr<SGeoGemGlyph> MyGlyph;
};
