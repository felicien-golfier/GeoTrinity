// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoSpeechPopupWidget.generated.h"

class FActiveTimerHandle;
class UTextBlock;
class UWidget;
enum class EActiveTimerReturnType : uint8;

/**
 * Base of every in-game chat popup, one line at a time: a new line replaces the shown one. The line types out, stays a
 * reading time set by its length once complete, then ends. The owning controller's OnInteractUI completes the text,
 * then ends the line. HitTestInvisible: it never takes a click. The skip key cap (SkipHint holding SkipKey) is part of
 * this base, so no popup can lack it. A child gives the content: it converts the line, shows the revealed part and
 * reacts to the talking state.
 */
UCLASS(Abstract)
class GEOTRINITYUI_API UGeoSpeechPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows Line, replacing the shown line, which ends first. OnEnded is called once when this line ends, whatever the
	 * reason: time, skip, a new line, destruction. */
	void ShowLine(FText const& Line, FSimpleDelegate OnEnded);

	/** Collapses the popup, calls the waiter. Nothing when no line shows. */
	void EndLine();

protected:
	/** Starts collapsed: nothing is said yet. Binds the owning controller's OnInteractUI to SkipLine. */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Child: prepares Line (conversion, words) and empties the shown text. Returns the number of visible characters.
	 */
	virtual int32 StartLine(FText const& Line) PURE_VIRTUAL(UGeoSpeechPopupWidget::StartLine, return 0;);

	/** Child: shows the first NewCount visible characters. PreviousCount were already shown. bTick is false when a skip
	 * reveals the rest. */
	virtual void RevealCharacters(int32 PreviousCount, int32 NewCount, bool bTick)
		PURE_VIRTUAL(UGeoSpeechPopupWidget::RevealCharacters, );

	/** Child: a line is on screen (true) or not (false). */
	virtual void SetTalking(bool /*bTalking*/) {}

	/** Reading time once the whole text shows, when nobody skips it. 0: set by the line's length (the .cpp's reading
	 * speed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSpeechPopup", meta = (ClampMin = "0", ClampMax = "30"))
	float LineSeconds = 0.f;

	/** The key cap at the panel's top right, visible while a line shows. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidget> SkipHint;

	/** The label of the key bound to InteractUIAction, set on each ShowLine. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
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

	/** Reveals up to Count visible characters. Starts the Reading phase once all show. */
	void RevealUpTo(int32 Count, bool bTick);

	/** Typing: shows the whole text. Reading: ends the line. Ended: nothing. */
	void SkipLine();

	/** The skip key's label: its key now, else its default key in the controller's mapping contexts, else the
	 * action's name. */
	FText GetSkipKeyLabel() const;

	EPhase Phase = EPhase::Ended;
	int32 VisibleLength = 0;
	int32 RevealedCount = 0;
	double PhaseStartTime = 0.;
	FSimpleDelegate OnLineEnded;
	TWeakPtr<FActiveTimerHandle> WatchTimer;
};
