// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoCatLineMarkup.h"

#include "GameplayTagsManager.h"

FString GeoCatLine::ToRichText(FString const& Markup, TFunctionRef<FText(FGameplayTag InputTag)> KeyLabel)
{
	FString Result;
	bool bInBold = false;
	bool bInShake = false;
	auto const ToggleRun = [&Result](TCHAR const* const RunName, bool& bInRun)
	{
		if (bInRun)
		{
			Result.Append(RunClose);
		}
		else
		{
			Result.AppendChar(RichTagOpen);
			Result.Append(RunName);
			Result.Append(RichTagClose);
		}
		bInRun = !bInRun;
	};
	int32 Index = 0;
	while (Index < Markup.Len())
	{
		int32 const TokenEnd = Markup[Index] == BraceOpen
			? Markup.Find(BraceClose, ESearchCase::CaseSensitive, ESearchDir::FromStart, Index)
			: INDEX_NONE;
		FString const Token = TokenEnd != INDEX_NONE ? Markup.Mid(Index + 1, TokenEnd - Index - 1) : FString();
		FGameplayTag const InputTag = Token.StartsWith(KeyTokenPrefix, ESearchCase::CaseSensitive)
			? UGameplayTagsManager::Get().RequestGameplayTag(FName(*Token.RightChop(KeyTokenPrefix.Len())), false)
			: FGameplayTag();

		if (FStringView(*Markup + Index).StartsWith(BoldMarker, ESearchCase::CaseSensitive))
		{
			ToggleRun(BoldRunName, bInBold);
			Index += BoldMarker.Len();
		}
		else if (FStringView(*Markup + Index).StartsWith(ShakeMarker, ESearchCase::CaseSensitive))
		{
			ToggleRun(ShakeRunName, bInShake);
			Index += ShakeMarker.Len();
		}
		else if (InputTag.IsValid())
		{
			Result += KeyLabel(InputTag).ToString();
			Index = TokenEnd + BraceClose.Len();
		}
		else
		{
			Result.AppendChar(Markup[Index]);
			++Index;
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
int32 GeoCatLine::TagLengthAt(FString const& RichText, int32 const Index)
{
	int32 const TagEnd = RichText[Index] == RichTagOpen
		? RichText.Find(RichTagClose, ESearchCase::CaseSensitive, ESearchDir::FromStart, Index)
		: INDEX_NONE;
	return TagEnd != INDEX_NONE ? TagEnd - Index + 1 : 0;
}

// ---------------------------------------------------------------------------------------------------------------------
FString GeoCatLine::VisibleText(FString const& RichText)
{
	FString Result;
	int32 Index = 0;
	while (Index < RichText.Len())
	{
		int32 const TagLength = TagLengthAt(RichText, Index);
		if (TagLength > 0)
		{
			Index += TagLength;
		}
		else
		{
			Result.AppendChar(RichText[Index]);
			++Index;
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
FString GeoCatLine::RevealRichText(FString const& RichText, int32 const VisibleCount)
{
	FString Result;
	bool bRunOpen = false;
	int32 Revealed = 0;
	int32 Index = 0;
	while (Index < RichText.Len() && Revealed < VisibleCount)
	{
		int32 const TagLength = TagLengthAt(RichText, Index);
		if (TagLength > 0)
		{
			Result += RichText.Mid(Index, TagLength);
			bRunOpen = RichText[Index + 1] != RichTagEnd;
			Index += TagLength;
		}
		else
		{
			Result.AppendChar(RichText[Index]);
			++Revealed;
			++Index;
		}
	}

	if (bRunOpen)
	{
		Result.Append(RunClose);
	}

	return Result;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FStringView> GeoCatLine::FindWords(FString const& Visible)
{
	TArray<FStringView> Words;
	int32 Index = 0;
	while (Index < Visible.Len())
	{
		if (FChar::IsWhitespace(Visible[Index]))
		{
			++Index;
		}
		else
		{
			int32 End = Index;
			while (End < Visible.Len() && !FChar::IsWhitespace(Visible[End]))
			{
				++End;
			}

			Words.Add(FStringView(*Visible + Index, End - Index));
			Index = End;
		}
	}

	return Words;
}

// ---------------------------------------------------------------------------------------------------------------------
float GeoCatLine::PitchForWord(FStringView const Word, float const Min, float const Max)
{
	constexpr uint32 Steps = 1000;
	uint32 const Hash = GetTypeHash(FString(Word).ToLower());
	return FMath::Lerp(Min, Max, static_cast<float>(Hash % Steps) / (Steps - 1));
}
