// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/Function.h"

namespace GeoCatLine
{
	/**
	 * Tutorial Board markup to URichTextBlock text: **x** to <Bold>x</>, ~~x~~ to <Shake>x</>, {Key:<InputTag>} to
	 * KeyLabel of that tag. Any other brace token, or one naming no existing tag, stays as written. No nesting.
	 */
	GEOTRINITYUI_API FString ToRichText(FString const& Markup, TFunctionRef<FText(FGameplayTag InputTag)> KeyLabel);

	/** Length of the rich-text tag (<Bold>, </>, ...) starting at Index of RichText; 0 when a visible character starts
	 * there. */
	GEOTRINITYUI_API int32 TagLengthAt(FString const& RichText, int32 Index);

	/** RichText without its tags: the characters the player reads. */
	GEOTRINITYUI_API FString VisibleText(FString const& RichText);

	/** The first VisibleCount visible characters of a ToRichText result, tags cost 0, the run left open is closed. */
	GEOTRINITYUI_API FString RevealRichText(FString const& RichText, int32 VisibleCount);

	/** The words of Visible (runs of non-whitespace characters), as views into Visible. */
	GEOTRINITYUI_API TArray<FStringView> FindWords(FString const& Visible);

	/** Pitch of Word in [Min, Max]: a hash of the word without regard to case, so the same word always gets the same
	 * one. */
	GEOTRINITYUI_API float PitchForWord(FStringView Word, float Min, float Max);
} // namespace GeoCatLine
