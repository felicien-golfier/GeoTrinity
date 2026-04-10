// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "Curves/CurveVector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AGeoGameCamera::AGeoGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGeoGameCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensureMsgf(CatchUpCurve, TEXT("AGeoGameCamera: CatchUpCurve is not set — camera will not move.")))
	{
		return;
	}

	// Gather all player pawn locations.
	FVector2D CentroidSum = FVector2D::ZeroVector;
	int32 PawnCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				FVector const PawnLocation = Pawn->GetActorLocation();
				CentroidSum += FVector2D(PawnLocation.X, PawnLocation.Y);
				++PawnCount;
			}
		}
	}

	if (PawnCount == 0)
	{
		return;
	}

	FVector2D const Centroid = CentroidSum / static_cast<float>(PawnCount);

	// Normalize the character position within the bounds: -1 at BoundsMin, 0 at center, +1 at BoundsMax.
	// Clamped so out-of-bounds characters don't produce values outside the curve range.
	FVector2D const BoundsCenter = (BoundsMin + BoundsMax) * 0.5f;
	FVector2D const BoundsHalfExtent = (BoundsMax - BoundsMin) * 0.5f;
	float const NormalizedX = FMath::Clamp((Centroid.X - BoundsCenter.X) / BoundsHalfExtent.X, -1.f, 1.f);
	float const NormalizedY = FMath::Clamp((Centroid.Y - BoundsCenter.Y) / BoundsHalfExtent.Y, -1.f, 1.f);

	// Sample per axis: curve designed to be high at 0 (full follow) and 0 at ±1 (stop at bounds).
	float const SpeedX = CatchUpCurve->GetVectorValue(NormalizedX).X * 1000;
	float const SpeedY = CatchUpCurve->GetVectorValue(NormalizedY).Y * 1000;

	// Target is the centroid clamped to the arena — camera never goes outside.
	FVector2D const Target = FVector2D(FMath::Clamp(Centroid.X, BoundsMin.X, BoundsMax.X),
									   FMath::Clamp(Centroid.Y, BoundsMin.Y, BoundsMax.Y));

	FVector const CurrentLocation = GetActorLocation();
	float const NewX = FMath::FInterpConstantTo(CurrentLocation.X, Target.X, DeltaTime, SpeedX);
	float const NewY = FMath::FInterpConstantTo(CurrentLocation.Y, Target.Y, DeltaTime, SpeedY);

	SetActorLocation(FVector(NewX, NewY, CurrentLocation.Z));
}
