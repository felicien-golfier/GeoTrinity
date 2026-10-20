// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoCatLineWidget.h"

#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "GameClasses/Player/GeoPlayerController.h"
#include "HUD/HudFunctionLibrary.h"
#include "HUD/Tutorial/GeoCatFaceWidget.h"
#include "HUD/Tutorial/GeoCatLineMarkup.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

constexpr float CharsPerSecond = 40.f;
constexpr float PitchMin = 0.8f;
constexpr float PitchMax = 1.4f;
/** Reading time once the text is complete: a base plus the length of the line at this reading speed. */
constexpr float ReadingBaseSeconds = 1.f;
constexpr float ReadingCharsPerSecond = 30.f;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	AGeoPlayerController* const PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer());
	if (ensureMsgf(PlayerController, TEXT("UGeoCatLineWidget: the owning player is not an AGeoPlayerController")))
	{
		PlayerController->OnInteractUI.AddUObject(this, &UGeoCatLineWidget::SkipLine);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::NativeDestruct()
{
	AGeoPlayerController* const PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer());
	if (PlayerController)
	{
		PlayerController->OnInteractUI.RemoveAll(this);
	}

	EndLine();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::ShowLine(FText const& Line, FSimpleDelegate OnEnded)
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

	AGeoPlayerController const* const PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer());
	FKey const SkipKeyBound = PlayerController
		? UHudFunctionLibrary::GetMappedKey(GetOwningLocalPlayer(), PlayerController->InteractUIAction)
		: FKey();
	if (SkipHint && SkipKey)
	{
		SkipHint->SetVisibility(SkipKeyBound.IsValid() ? ESlateVisibility::HitTestInvisible
													   : ESlateVisibility::Collapsed);
		SkipKey->SetText(SkipKeyBound.GetDisplayName(false));
	}

	VisibleLine = GeoCatLine::VisibleText(RichLine);
	Words = GeoCatLine::FindWords(VisibleLine);

	FSimpleDelegate const ReplacedWaiter = MoveTemp(OnLineEnded);
	OnLineEnded = MoveTemp(OnEnded);
	Phase = EPhase::Typing;
	RevealedCount = 0;
	PhaseStartTime = FSlateApplication::Get().GetCurrentTime();
	LineText->SetText(FText::GetEmpty());
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (CatFace)
	{
		CatFace->SetTalking(true);
	}

	if (!WatchTimer.IsValid())
	{
		WatchTimer = TakeWidget()->RegisterActiveTimer(
			0.f, FWidgetActiveTimerDelegate::CreateUObject(this, &UGeoCatLineWidget::WatchLine));
	}

	ReplacedWaiter.ExecuteIfBound();
}

// ---------------------------------------------------------------------------------------------------------------------
EActiveTimerReturnType UGeoCatLineWidget::WatchLine(double const CurrentTime, float /*DeltaTime*/)
{
	if (Phase == EPhase::Typing)
	{
		RevealCharacters(FMath::FloorToInt((CurrentTime - PhaseStartTime) * CharsPerSecond), true);
	}
	else if (Phase == EPhase::Reading)
	{
		float const ReadingSeconds =
			LineSeconds > 0.f ? LineSeconds : ReadingBaseSeconds + VisibleLine.Len() / ReadingCharsPerSecond;
		if (CurrentTime - PhaseStartTime >= ReadingSeconds)
		{
			EndLine();
		}
	}

	return Phase == EPhase::Ended ? EActiveTimerReturnType::Stop : EActiveTimerReturnType::Continue;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::RevealCharacters(int32 const Count, bool const bTick)
{
	int32 const NewCount = FMath::Min(Count, VisibleLine.Len());
	if (NewCount > RevealedCount)
	{
		int32 const LastNewWord = Words.FindLastByPredicate(
			[this, NewCount](FStringView const& Word)
			{
				return Word.GetData() - *VisibleLine >= RevealedCount && Word.GetData() - *VisibleLine < NewCount;
			});
		RevealedCount = NewCount;
		LineText->SetText(FText::FromString(GeoCatLine::RevealRichText(RichLine, RevealedCount)));
		if (bTick && Words.IsValidIndex(LastNewWord) && TypeSound)
		{
			UGameplayStatics::PlaySound2D(this, TypeSound, 1.f,
										  GeoCatLine::PitchForWord(Words[LastNewWord], PitchMin, PitchMax));
		}
	}

	if (RevealedCount >= VisibleLine.Len())
	{
		Phase = EPhase::Reading;
		PhaseStartTime = FSlateApplication::Get().GetCurrentTime();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::SkipLine()
{
	if (Phase == EPhase::Typing)
	{
		RevealCharacters(VisibleLine.Len(), false);
	}
	else if (Phase == EPhase::Reading)
	{
		EndLine();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCatLineWidget::EndLine()
{
	if (Phase != EPhase::Ended)
	{
		Phase = EPhase::Ended;
		SetVisibility(ESlateVisibility::Collapsed);
		if (CatFace)
		{
			CatFace->SetTalking(false);
		}

		FSimpleDelegate const Waiter = MoveTemp(OnLineEnded);
		Waiter.ExecuteIfBound();
	}
}
