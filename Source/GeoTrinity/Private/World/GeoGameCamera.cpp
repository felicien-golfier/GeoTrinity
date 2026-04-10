// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "Curves/CurveLinearColor.h"
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
	FVector2D const Target = FVector2D(
		FMath::Clamp(Centroid.X, BoundsMin.X, BoundsMax.X),
		FMath::Clamp(Centroid.Y, BoundsMin.Y, BoundsMax.Y));

	FVector const CurrentLocation = GetActorLocation();
	FVector2D const CurrentXY = FVector2D(CurrentLocation.X, CurrentLocation.Y);
	FVector2D const Delta = Target - CurrentXY;

	float const DistanceX = FMath::Abs(Delta.X);
	float const DistanceY = FMath::Abs(Delta.Y);

	// Sample the catch-up curve: R = X-axis speed, G = Y-axis speed.
	FLinearColor const CurveValueX = CatchUpCurve->GetLinearColorValue(DistanceX);
	FLinearColor const CurveValueY = CatchUpCurve->GetLinearColorValue(DistanceY);
	float const SpeedX = CurveValueX.R;
	float const SpeedY = CurveValueY.G;

	float const NewX = FMath::FInterpConstantTo(CurrentXY.X, Target.X, DeltaTime, SpeedX);
	float const NewY = FMath::FInterpConstantTo(CurrentXY.Y, Target.Y, DeltaTime, SpeedY);

	SetActorLocation(FVector(NewX, NewY, CurrentLocation.Z));
}
