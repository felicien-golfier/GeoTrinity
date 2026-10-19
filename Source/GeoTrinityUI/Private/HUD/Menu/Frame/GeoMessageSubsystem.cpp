// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/Frame/GeoMessageSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Tool/Visual/GeoColor.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMessageSubsystem::Post(UObject const* WorldContext, FText const& Text, EGeoColor const Color)
{
	UWorld const* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance const* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (UGeoMessageSubsystem* Messages = GameInstance ? GameInstance->GetSubsystem<UGeoMessageSubsystem>() : nullptr)
	{
		Messages->PostMessage(Text, Color);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMessageSubsystem::PostMessage(FText const& Text, EGeoColor const Color)
{
	double const Now = FPlatformTime::Seconds();
	bool const bDuplicate = !Messages.IsEmpty() && Messages.Last().Text.ToString() == Text.ToString()
		&& Now - Messages.Last().PostTime < DuplicateSeconds;
	if (!bDuplicate)
	{
		Messages.Add({Text, Color, Now});
		OnMessagePosted.Broadcast();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMessageSubsystem::Clear()
{
	Messages.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMessageSubsystem::Prune(double const Now, FGeoMessageLayoutParams const& Params)
{
	TArray<double> const Expiries = ComputeExpiries(Messages, Params);
	int32 Gone = 0;
	while (Gone < Expiries.Num() && Expiries[Gone] + Params.FadeTime + Params.SlideTime < Now)
	{
		++Gone;
	}

	Messages.RemoveAt(0, Gone);
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<double> UGeoMessageSubsystem::ComputeExpiries(TArray<FGeoMessage> const& Messages,
													 FGeoMessageLayoutParams const& Params)
{
	TArray<double> Expiries;
	for (FGeoMessage const& Message : Messages)
	{
		int32 LiveCount = 0;
		int32 Oldest = INDEX_NONE;
		for (int32 Index = 0; Index < Expiries.Num(); ++Index)
		{
			if (Expiries[Index] > Message.PostTime)
			{
				++LiveCount;
				Oldest = Oldest == INDEX_NONE ? Index : Oldest;
			}
		}

		if (LiveCount >= Params.MaxLines && Oldest != INDEX_NONE)
		{
			Expiries[Oldest] = Message.PostTime;
		}

		Expiries.Add(Message.PostTime + Params.Lifetime);
	}

	return Expiries;
}

// ---------------------------------------------------------------------------------------------------------------------
TArray<FGeoMessageLine> UGeoMessageSubsystem::Layout(TArray<FGeoMessage> const& Messages, double const Now,
													 FGeoMessageLayoutParams const& Params)
{
	TArray<double> const Expiries = ComputeExpiries(Messages, Params);
	TArray<FGeoMessageLine> Lines;
	for (int32 Index = 0; Index < Messages.Num(); ++Index)
	{
		float const FadeIn =
			Params.FadeTime > 0.f ? FMath::Clamp((Now - Messages[Index].PostTime) / Params.FadeTime, 0., 1.) : 1.f;
		float const FadeOut = Params.FadeTime > 0.f
			? 1.f - FMath::Clamp((Now - Expiries[Index]) / Params.FadeTime, 0., 1.)
			: (Now < Expiries[Index] ? 1.f : 0.f);
		float const Alpha = Now >= Messages[Index].PostTime ? FadeIn * FadeOut : 0.f;
		if (Alpha > 0.f)
		{
			float Slot = 0.f;
			for (int32 Older = 0; Older < Index; ++Older)
			{
				float const Left = Params.SlideTime > 0.f
					? 1.f - FMath::SmoothStep(0., 1., (Now - Expiries[Older]) / Params.SlideTime)
					: (Now < Expiries[Older] ? 1.f : 0.f);
				Slot += Left;
			}

			Lines.Add({Index, Slot, Alpha});
		}
	}

	return Lines;
}
