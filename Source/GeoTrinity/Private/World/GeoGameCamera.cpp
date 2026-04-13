// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "Camera/CameraComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AGeoGameCamera::AGeoGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGeoGameCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensureMsgf(FollowSpeedCurve, TEXT("AGeoGameCamera: FollowSpeedCurve is not set — camera will not move.")))
	{
		return;
	}

	// Use only the local player's pawn — each client drives its own camera.
	APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();
	if (!LocalPlayerController)
	{
		return;
	}

	APawn* LocalPawn = LocalPlayerController->GetPawn();
	if (!LocalPawn)
	{
		return;
	}

	FVector const PawnLocation = LocalPawn->GetActorLocation();
	FVector2D const Centroid = FVector2D(PawnLocation.X, PawnLocation.Y);

	// Compute orthographic half-extents from the camera component.
	float const OrthoHalfWidth = GetCameraComponent()->OrthoWidth * 0.5f;

	float AspectRatio = 16.f / 9.f;
	if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
	{
		FVector2D ViewportSize;
		ViewportClient->GetViewportSize(ViewportSize);
		if (ViewportSize.X > 0.f && ViewportSize.Y > 0.f)
		{
			AspectRatio = ViewportSize.X / ViewportSize.Y;
		}
	}
	float const OrthoHalfHeight = OrthoHalfWidth / AspectRatio;

	// Character position relative to camera center.
	FVector2D const CameraPosition = FVector2D(GetActorLocation());
	FVector2D const RelativePosition = Centroid - CameraPosition;

	// The trigger zone starts at (1 - ScreenEdgeThresholdPercent) of the half-extent from the camera center.
	float const EdgeZoneHalfWidth = OrthoHalfWidth * ScreenEdgeThresholdPercent;
	float const EdgeZoneHalfHeight = OrthoHalfHeight * ScreenEdgeThresholdPercent;
	float const TriggerThresholdX = OrthoHalfWidth - EdgeZoneHalfWidth;
	float const TriggerThresholdY = OrthoHalfHeight - EdgeZoneHalfHeight;

	// Edge proximity: 0 when character just entered the trigger zone, 1 when at the screen edge.
	// Zero when the character is well within the screen — no movement triggered.
	float const ProximityX = FMath::Clamp((FMath::Abs(RelativePosition.X) - TriggerThresholdX) / EdgeZoneHalfWidth, 0.f, 1.f);
	float const ProximityY = FMath::Clamp((FMath::Abs(RelativePosition.Y) - TriggerThresholdY) / EdgeZoneHalfHeight, 0.f, 1.f);

	float const SpeedX = FollowSpeedCurve->GetVectorValue(ProximityX).X;
	float const SpeedY = FollowSpeedCurve->GetVectorValue(ProximityY).Y;

	// Valid camera position range: ensures map bounds never appear inside the viewport.
	// When map is smaller than the visible area on an axis, center on that axis.
	float const MinCameraX = BoundsMin.X + OrthoHalfWidth;
	float const MaxCameraX = BoundsMax.X - OrthoHalfWidth;
	float const MinCameraY = BoundsMin.Y + OrthoHalfHeight;
	float const MaxCameraY = BoundsMax.Y - OrthoHalfHeight;

	FVector2D const BoundsCenter = (BoundsMin + BoundsMax) * 0.5f;
	FVector2D const Target = FVector2D(
		MinCameraX <= MaxCameraX ? FMath::Clamp(Centroid.X, MinCameraX, MaxCameraX) : BoundsCenter.X,
		MinCameraY <= MaxCameraY ? FMath::Clamp(Centroid.Y, MinCameraY, MaxCameraY) : BoundsCenter.Y
	);

	FVector const CurrentLocation = GetActorLocation();
	float const NewX = FMath::FInterpConstantTo(CurrentLocation.X, Target.X, DeltaTime, SpeedX);
	float const NewY = FMath::FInterpConstantTo(CurrentLocation.Y, Target.Y, DeltaTime, SpeedY);

	SetActorLocation(FVector(NewX, NewY, CurrentLocation.Z));
}
