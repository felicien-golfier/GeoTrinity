// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoCatLineWidget.generated.h"

class FActiveTimerHandle;
class UGeoCatFaceWidget;
class URichTextBlock;
class USoundBase;
class UTextBlock;
class UWidget;
enum class EActiveTimerReturnType : uint8;

/**
 * The Cat's line on screen (WBP_CatLine), one at a time: a new line replaces the shown one. It types out letter by
 * letter (a tick per word), stays a reading time set by its length once complete, then
 * ends. The owning controller's OnInteractUI completes
 * the text, then ends the line (no mouse button: the mouse fires abilities). HitTestInvisible: it never takes a click.
 * Required in the BP hierarchy: URichTextBlock "LineText", its style table holding a "Bold" row and its decorators a
 * UGeoShakeTextDecorator. Optional: UGeoCatFaceWidget "CatFace", talking while a line shows; UWidget "SkipHint" holding
 * UTextBlock "SkipKey", the key that skips the line.
 */
UCLASS()
class GEOTRINITYUI_API UGeoCatLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Line, written in the Tutorial Board markup (GeoCatLineMarkup.h), its key tokens read from the owning
	 * player's bindings. Replaces the shown line, which ends first. OnEnded is called once when this line ends,
	 * whatever the reason: time, skip, a new line, destruction. */
	void ShowLine(FText const& Line, FSimpleDelegate OnEnded);

protected:
	/** Starts collapsed: nothing is said yet. Binds the owning controller's OnInteractUI to SkipLine. */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<URichTextBlock> LineText;

	/** Reading time once the whole text shows, when nobody skips it. 0: set by the line's length (the .cpp's reading
	 * speed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatLine", meta = (ClampMin = "0", ClampMax = "30"))
	float LineSeconds = 0.f;

	/** The tick played when a word's first letter appears. Null: silent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCatLine")
	TObjectPtr<USoundBase> TypeSound;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoCatFaceWidget> CatFace;

	/** The key cap at the panel's top right: collapsed when the skip key is unbound. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SkipHint;

	/** The label of the key bound to InteractUIAction, set on each ShowLine. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkipKey;

private:
	enum class EPhase : uint8
	{
		Typing,
		Reading,
		Ended
	};

	/** Types the text out, then ends the line once its reading time has passed. */
	EActiveTimerReturnType WatchLine(double CurrentTime, float DeltaTime);

	/** Shows the first Count visible characters; plays the tick of the last word that starts among the new ones when
	 * bTick. Starts the Reading phase once all show. */
	void RevealCharacters(int32 Count, bool bTick);

	/** Typing: shows the whole text. Reading: ends the line. Ended: nothing. */
	void SkipLine();

	/** Collapses the line, silences the face, calls the waiter. Nothing when no line shows. */
	void EndLine();

	EPhase Phase = EPhase::Ended;
	FString RichLine;
	FString VisibleLine;
	TArray<FStringView> Words;
	int32 RevealedCount = 0;
	double PhaseStartTime = 0.;
	FSimpleDelegate OnLineEnded;
	TWeakPtr<FActiveTimerHandle> WatchTimer;
};
