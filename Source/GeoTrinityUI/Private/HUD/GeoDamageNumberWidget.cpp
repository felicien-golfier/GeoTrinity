// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoDamageNumberWidget.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::Activate(float Amount, EGeoDamageNumberType Type, FVector InWorldPos)
{
	WorldPos = InWorldPos;
	WorldPos.X += FMath::RandRange(-LocationStartDrift, LocationStartDrift);
	WorldPos.Y += FMath::RandRange(-LocationStartDrift, LocationStartDrift);
	FVector2D const Dir(FMath::RandRange(-1.f, 1.f), -1.f);
	DriftOffset = Dir.GetSafeNormal() * DriftDistance;

	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	ApplyDriftAndFade(0.f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetData(Amount, Type);
	GetWorld()->GetTimerManager().SetTimer(LifetimeTimerHandle, this, &UGeoDamageNumberWidget::ReturnToPool,
										   VisibleDuration);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDamageNumberWidget::IsAvailable() const
{
	return !GetWorld()->GetTimerManager().IsTimerActive(LifetimeTimerHandle);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	float const ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(LifetimeTimerHandle);
	if (ElapsedTime >= 0.f)
	{
		ApplyDriftAndFade(FMath::Clamp(ElapsedTime / VisibleDuration, 0.f, 1.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::ReturnToPool()
{
	GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::ApplyDriftAndFade(float const Alpha)
{
	FVector2D ScreenPos;
	if (ProjectToScreen(ScreenPos))
	{
		SetPositionInViewport(ScreenPos + DriftOffset * Alpha, true);
	}

	SetRenderOpacity(1.f - Alpha);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoDamageNumberWidget::ProjectToScreen(FVector2D& OutScreenPos) const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, true))
	{
		return false;
	}

	// bPlayerViewportRelative=true returns coords inside the constrained game image (inside black bars); re-add the
	// image origin so SetPositionInViewport (full window space) lands in the right place.
	if (ULocalPlayer const* LP = PC->GetLocalPlayer())
	{
		FSceneViewProjectionData ProjectionData;
		if (LP->GetProjectionData(LP->ViewportClient->Viewport, ProjectionData))
		{
			OutScreenPos += FVector2D(ProjectionData.GetConstrainedViewRect().Min);
		}
	}
	return true;
}
