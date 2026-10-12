// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/Frame/GeoButton.h"

#include "GeoGemSocketButton.generated.h"

class UGeoGemGlyph;

DECLARE_DELEGATE_OneParam(FGeoGemSocketPickedSignature, int32 /*SocketIndex*/);
DECLARE_DELEGATE_TwoParams(FGeoGemSocketHoverChangedSignature, int32 /*SocketIndex*/, bool /*bHovered*/);

/**
 * One socket of the gem board: a bare button whose whole look is its UGeoGemGlyph, grown under the cursor or the
 * gamepad focus, and which says which socket it is when clicked — UMG's click delegate carries nothing.
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemSocketButton : public UGeoButton
{
	GENERATED_BODY()

public:
	/** Strips the button's own brushes and wraps it around InGlyph, socket InSocketIndex of GeoGem::GetSockets(). */
	void InitSocket(int32 InSocketIndex, UGeoGemGlyph* InGlyph);

	/** Returns the glyph widget this socket button wraps. */
	UGeoGemGlyph* GetGlyph() const { return Glyph; }

	FGeoGemSocketPickedSignature OnPicked;

	/** The cursor or the gamepad focus entering (true) or leaving (false) the socket. */
	FGeoGemSocketHoverChangedSignature OnHoverChanged;

	/** Scale of the glyph under the cursor or the gamepad focus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "1", ClampMax = "2"))
	float HoverScale = 1.12f;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	int32 SocketIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UGeoGemGlyph> Glyph;
};
