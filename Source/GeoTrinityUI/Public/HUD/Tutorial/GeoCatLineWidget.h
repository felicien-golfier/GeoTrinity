// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Tutorial/GeoSpeechPopupWidget.h"

#include "GeoCatLineWidget.generated.h"

class UGeoCatFaceWidget;
class URichTextBlock;
class USoundBase;

/**
 * The Cat's line on screen (WBP_CatLine): the speech popup (GeoSpeechPopupWidget.h) with the Cat's content. The line is
 * written in the Tutorial Board markup (GeoCatLineMarkup.h) and types out letter by letter with a tick per word.
 * Required in the BP hierarchy: URichTextBlock "LineText", its style table holding a "Bold" row and its decorators a
 * UGeoShakeTextDecorator. Optional: UGeoCatFaceWidget "CatFace", talking while a line shows.
 */
UCLASS()
class GEOTRINITYUI_API UGeoCatLineWidget : public UGeoSpeechPopupWidget
{
	GENERATED_BODY()

protected:
	/** Converts Line to rich text, its key tokens read from the owning player's bindings; finds the words. */
	virtual int32 StartLine(FText const& Line) override;

	/** Shows the revealed rich text; plays the tick of the last word that starts among the new characters. */
	virtual void RevealCharacters(int32 PreviousCount, int32 NewCount, bool bTick) override;

	virtual void SetTalking(bool bTalking) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<URichTextBlock> LineText;

	/** The tick played when a word's first letter appears. Null: silent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatLine")
	TObjectPtr<USoundBase> TypeSound;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoCatFaceWidget> CatFace;

private:
	FString RichLine;
	FString VisibleLine;
	TArray<FStringView> Words;
};
