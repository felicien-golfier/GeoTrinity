// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoSpeechPopupWidget.h"

#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "GameClasses/Player/GeoPlayerController.h"
#include "HUD/HudFunctionLibrary.h"
#include "InputAction.h"

constexpr float CharsPerSecond = 40.f;
/** Reading time once the text is complete: a base plus the length of the line at this reading speed. */
constexpr float ReadingBaseSeconds = 1.f;
constexpr float ReadingCharsPerSecond = 30.f;

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSpeechPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	AGeoPlayerController* const PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer());
	if (ensureMsgf(PlayerController, TEXT("UGeoSpeechPopupWidget: the owning player is not an AGeoPlayerController")))
	{
		PlayerController->OnInteractUI.AddUObject(this, &UGeoSpeechPopupWidget::SkipLine);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSpeechPopupWidget::NativeDestruct()
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
void UGeoSpeechPopupWidget::ShowLine(FText const& Line, FSimpleDelegate OnEnded)
{
	SkipKey->SetText(GetSkipKeyLabel());
	SkipHint->SetVisibility(ESlateVisibility::HitTestInvisible);

	VisibleLength = StartLine(Line);

	FSimpleDelegate const ReplacedWaiter = MoveTemp(OnLineEnded);
	OnLineEnded = MoveTemp(OnEnded);
	Phase = EPhase::Typing;
	RevealedCount = 0;
	PhaseStartTime = FSlateApplication::Get().GetCurrentTime();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetTalking(true);

	if (!WatchTimer.IsValid())
	{
		WatchTimer = TakeWidget()->RegisterActiveTimer(
			0.f, FWidgetActiveTimerDelegate::CreateUObject(this, &UGeoSpeechPopupWidget::WatchLine));
	}

	ReplacedWaiter.ExecuteIfBound();
}

// ---------------------------------------------------------------------------------------------------------------------
EActiveTimerReturnType UGeoSpeechPopupWidget::WatchLine(double const CurrentTime, float /*DeltaTime*/)
{
	if (Phase == EPhase::Typing)
	{
		RevealUpTo(FMath::FloorToInt((CurrentTime - PhaseStartTime) * CharsPerSecond), true);
	}
	else if (Phase == EPhase::Reading)
	{
		float const ReadingSeconds =
			LineSeconds > 0.f ? LineSeconds : ReadingBaseSeconds + VisibleLength / ReadingCharsPerSecond;
		if (CurrentTime - PhaseStartTime >= ReadingSeconds)
		{
			EndLine();
		}
	}

	return Phase == EPhase::Ended ? EActiveTimerReturnType::Stop : EActiveTimerReturnType::Continue;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSpeechPopupWidget::RevealUpTo(int32 const Count, bool const bTick)
{
	int32 const NewCount = FMath::Min(Count, VisibleLength);
	if (NewCount > RevealedCount)
	{
		RevealCharacters(RevealedCount, NewCount, bTick);
		RevealedCount = NewCount;
	}

	if (RevealedCount >= VisibleLength)
	{
		Phase = EPhase::Reading;
		PhaseStartTime = FSlateApplication::Get().GetCurrentTime();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSpeechPopupWidget::SkipLine()
{
	if (Phase == EPhase::Typing)
	{
		RevealUpTo(VisibleLength, false);
	}
	else if (Phase == EPhase::Reading)
	{
		EndLine();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSpeechPopupWidget::EndLine()
{
	if (Phase != EPhase::Ended)
	{
		Phase = EPhase::Ended;
		SetVisibility(ESlateVisibility::Collapsed);
		SetTalking(false);

		FSimpleDelegate const Waiter = MoveTemp(OnLineEnded);
		Waiter.ExecuteIfBound();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
FText UGeoSpeechPopupWidget::GetSkipKeyLabel() const
{
	AGeoPlayerController const* const PlayerController = Cast<AGeoPlayerController>(GetOwningPlayer());
	if (!ensureMsgf(PlayerController && PlayerController->InteractUIAction,
					TEXT("UGeoSpeechPopupWidget: no controller or InteractUIAction")))
	{
		return FText::GetEmpty();
	}

	UInputAction const* const Action = PlayerController->InteractUIAction;
	FKey Key = UHudFunctionLibrary::GetMappedKey(GetOwningLocalPlayer(), Action);
	if (!Key.IsValid())
	{
		Key = UHudFunctionLibrary::GetDefaultMappedKey(PlayerController->InputMapping, Action);
	}

	if (!Key.IsValid())
	{
		Key = UHudFunctionLibrary::GetDefaultMappedKey(PlayerController->MenuInputMapping, Action);
	}

	return Key.IsValid() ? Key.GetDisplayName(false) : FText::FromName(Action->GetFName());
}
