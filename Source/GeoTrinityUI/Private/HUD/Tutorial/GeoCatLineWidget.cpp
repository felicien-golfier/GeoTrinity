// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoCatLineWidget.h"

#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/RichTextBlock.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Tutorial/GeoCatFaceWidget.h"
#include "HUD/Tutorial/GeoCatLineMarkup.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

constexpr float PitchMin = 0.8f;
constexpr float PitchMax = 1.4f;

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoCatLineWidget::StartLine(FText const& Line)
{
	UAbilityInfo const* const AbilityInfo = GeoASLib::GetAbilityInfo();
	TArray<FPlayersGameplayAbilityInfo> const PlayerAbilities =
		AbilityInfo ? AbilityInfo->GetAllPlayersAbilityInfos() : TArray<FPlayersGameplayAbilityInfo>();
	RichLine = GeoCatLine::ToRichText(
		Line.ToString(),
		[this, &PlayerAbilities](FGameplayTag const InputTag)
		{
			FPlayersGameplayAbilityInfo const* const Info = PlayerAbilities.FindByPredicate(
				[InputTag](FPlayersGameplayAbilityInfo const& Candidate)
				{
					return Candidate.InputTag == InputTag;
				});
			FKey const Key =
				UHudFunctionLibrary::GetMappedKey(GetOwningLocalPlayer(), Info ? Info->InputAction : nullptr);
			return Key.IsValid() ? Key.GetDisplayName(false) : FText::FromName(InputTag.GetTagName());
		});

	VisibleLine = GeoCatLine::VisibleText(RichLine);
	Words = GeoCatLine::FindWords(VisibleLine);
	LineText->SetText(FText::GetEmpty());
	return VisibleLine.Len();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::RevealCharacters(int32 const PreviousCount, int32 const NewCount, bool const bTick)
{
	int32 const LastNewWord = Words.FindLastByPredicate(
		[this, PreviousCount, NewCount](FStringView const& Word)
		{
			return Word.GetData() - *VisibleLine >= PreviousCount && Word.GetData() - *VisibleLine < NewCount;
		});
	LineText->SetText(FText::FromString(GeoCatLine::RevealRichText(RichLine, NewCount)));
	if (bTick && Words.IsValidIndex(LastNewWord) && TypeSound)
	{
		UGameplayStatics::PlaySound2D(this, TypeSound, 1.f,
									  GeoCatLine::PitchForWord(Words[LastNewWord], PitchMin, PitchMax));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::SetTalking(bool const bTalking)
{
	if (CatFace)
	{
		CatFace->SetTalking(bTalking);
	}
}
