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
	FVector2D const CharacterPosition = FVector2D(PawnLocation.X, PawnLocation.Y);

	// Camera screen axes projected onto the world XY plane (camera looks straight down, Z is irrelevant).
	// With pitch=-90, yaw=0: ScreenRight = World Y (horizontal), ScreenUp = World X (vertical).
	// Using actual camera vectors makes this correct for any yaw.
	FVector2D const ScreenRight = FVector2D(GetActorRightVector()).GetSafeNormal();
	FVector2D const ScreenUp = FVector2D(GetActorUpVector()).GetSafeNormal();

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

	// Project character offset onto screen axes to get true screen-space distances.
	FVector2D const CameraPosition = FVector2D(GetActorLocation());
	FVector2D const RelativePosition = CharacterPosition - CameraPosition;
	float const ScreenOffsetH = FVector2D::DotProduct(RelativePosition, ScreenRight);
	float const ScreenOffsetV = FVector2D::DotProduct(RelativePosition, ScreenUp);

	// Edge proximity: 0 when character just entered the trigger zone, 1 when at the screen edge.
	float const EdgeZoneHalfWidth = OrthoHalfWidth * ScreenEdgeThresholdPercent;
	float const EdgeZoneHalfHeight = OrthoHalfHeight * ScreenEdgeThresholdPercent;

	float const ProximityH = FMath::Clamp((FMath::Abs(ScreenOffsetH) - (OrthoHalfWidth - EdgeZoneHalfWidth)) / EdgeZoneHalfWidth, 0.f, 1.f);
	float const ProximityV = FMath::Clamp((FMath::Abs(ScreenOffsetV) - (OrthoHalfHeight - EdgeZoneHalfHeight)) / EdgeZoneHalfHeight, 0.f, 1.f);

	float const SpeedH = FollowSpeedCurve->GetVectorValue(ProximityH).X;
	float const SpeedV = FollowSpeedCurve->GetVectorValue(ProximityV).Y;

	// World-space AABB half-extents of the viewport rectangle.
	// For yaw=0: ExtentX = OrthoHalfHeight, ExtentY = OrthoHalfWidth. General formula handles any yaw.
	float const ViewportHalfExtentX = OrthoHalfWidth * FMath::Abs(ScreenRight.X) + OrthoHalfHeight * FMath::Abs(ScreenUp.X);
	float const ViewportHalfExtentY = OrthoHalfWidth * FMath::Abs(ScreenRight.Y) + OrthoHalfHeight * FMath::Abs(ScreenUp.Y);

	// Valid camera position range: ensures map bounds stay outside the viewport.
	// When the map is smaller than the visible area on an axis, center on that axis.
	float const MinCameraX = BoundsMin.X + ViewportHalfExtentX;
	float const MaxCameraX = BoundsMax.X - ViewportHalfExtentX;
	float const MinCameraY = BoundsMin.Y + ViewportHalfExtentY;
	float const MaxCameraY = BoundsMax.Y - ViewportHalfExtentY;

	FVector2D const BoundsCenter = (BoundsMin + BoundsMax) * 0.5f;
	FVector2D const Target = FVector2D(
		MinCameraX <= MaxCameraX ? FMath::Clamp(CharacterPosition.X, MinCameraX, MaxCameraX) : BoundsCenter.X,
		MinCameraY <= MaxCameraY ? FMath::Clamp(CharacterPosition.Y, MinCameraY, MaxCameraY) : BoundsCenter.Y
	);

	// Move in screen space (along camera axes), then convert back to world XY.
	FVector2D const CameraToTarget = Target - CameraPosition;
	float const DeltaH = FMath::FInterpConstantTo(0.f, FVector2D::DotProduct(CameraToTarget, ScreenRight), DeltaTime, SpeedH);
	float const DeltaV = FMath::FInterpConstantTo(0.f, FVector2D::DotProduct(CameraToTarget, ScreenUp), DeltaTime, SpeedV);

	FVector2D const NewPosition = CameraPosition + ScreenRight * DeltaH + ScreenUp * DeltaV;
	FVector const CurrentLocation = GetActorLocation();
	SetActorLocation(FVector(NewPosition.X, NewPosition.Y, CurrentLocation.Z));
}
