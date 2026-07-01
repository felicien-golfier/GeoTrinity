// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/GeoDamageNumberWidget.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::Activate(float Amount, bool bIsHeal, FVector InWorldPos)
{
	WorldPos = InWorldPos;
	WorldPos.X += FMath::RandRange(-LocationStartDrift, LocationStartDrift);
	WorldPos.Y += FMath::RandRange(-LocationStartDrift, LocationStartDrift);
	FVector2D const Dir(FMath::RandRange(-1.f, 1.f), -1.f);
	DriftOffset = Dir.GetSafeNormal() * DriftDistance;
	ElapsedTime = 0.f;
	bActive = true;
	bAvailable = false;

	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetRenderOpacity(0.f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetData(Amount, bIsHeal);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bActive)
	{
		return;
	}

	ElapsedTime += InDeltaTime;
	float const Alpha = FMath::Clamp(ElapsedTime / VisibleDuration, 0.f, 1.f);

	FVector2D ScreenPos;
	if (ProjectToScreen(ScreenPos))
	{
		SetPositionInViewport(ScreenPos + DriftOffset * Alpha, true);
	}
	SetRenderOpacity(1.f - Alpha);

	if (Alpha >= 1.f)
	{
		ReturnToPool();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoDamageNumberWidget::ReturnToPool()
{
	bActive = false;
	bAvailable = true;
	SetVisibility(ESlateVisibility::Collapsed);
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
