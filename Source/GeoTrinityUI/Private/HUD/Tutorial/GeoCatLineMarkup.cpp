// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoCatLineMarkup.h"

#include "GameplayTagsManager.h"

FString GeoCatLine::ToRichText(FString const& Markup, TFunctionRef<FText(FGameplayTag InputTag)> KeyLabel)
{
	FString const KeyPrefix = TEXT("Key:");
	FString Result;
	bool bInBold = false;
	bool bInShake = false;
	int32 Index = 0;
	while (Index < Markup.Len())
	{
		int32 const TokenEnd = Markup[Index] == TCHAR('{')
			? Markup.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index)
			: INDEX_NONE;
		FString const Token = TokenEnd != INDEX_NONE ? Markup.Mid(Index + 1, TokenEnd - Index - 1) : FString();
		FGameplayTag const InputTag = Token.StartsWith(KeyPrefix, ESearchCase::CaseSensitive)
			? UGameplayTagsManager::Get().RequestGameplayTag(FName(*Token.RightChop(KeyPrefix.Len())), false)
			: FGameplayTag();

		if (Markup.Mid(Index, 2) == TEXT("**"))
		{
			Result += bInBold ? TEXT("</>") : TEXT("<Bold>");
			bInBold = !bInBold;
			Index += 2;
		}
		else if (Markup.Mid(Index, 2) == TEXT("~~"))
		{
			Result += bInShake ? TEXT("</>") : TEXT("<Shake>");
			bInShake = !bInShake;
			Index += 2;
		}
		else if (InputTag.IsValid())
		{
			Result += KeyLabel(InputTag).ToString();
			Index = TokenEnd + 1;
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
	int32 const TagEnd = RichText[Index] == TCHAR('<')
		? RichText.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index)
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
			bRunOpen = RichText[Index + 1] != TCHAR('/');
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
		Result += TEXT("</>");
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
