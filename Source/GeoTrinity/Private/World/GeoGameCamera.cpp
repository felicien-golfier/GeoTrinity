// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerController.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoGameCamera::AGeoGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGeoGameCamera::BeginPlay()
{
	Super::BeginPlay();

	AGeoGameState* GameState = GetWorld()->GetGameState<AGeoGameState>();
	GameState->CommitFightDelegate.AddUniqueDynamic(this, &AGeoGameCamera::CalculateBounds);
	GameState->OnMatchStateChanged.AddUniqueDynamic(this, &AGeoGameCamera::OnMatchStateChanged);
	CalculateBounds();
}

void AGeoGameCamera::OnMatchStateChanged(FName MatchState, FName PreviousMatchState)
{
	if (PreviousMatchState == MatchState)
	{
		return;
	}

	// Let Commit Fight update the bounds
	if (MatchState == MatchState::InProgress)
	{
		return;
	}

	CalculateBounds();
}

void AGeoGameCamera::CalculateBounds()
{
	AGameState const* GameState = GetWorld()->GetGameState<AGameState>();
	bool const bFightInProgress = GameState && GameState->GetMatchState() == MatchState::InProgress;
	FGameplayTag const BoundsTag =
		bFightInProgress ? FGeoGameplayTags::Get().Camera_Bounds_Fight : FGeoGameplayTags::Get().Camera_Bounds_Intro;

	TArray<AActor*> BoundPoints = UGeoGameplayLibrary::GetTargetPoints(this, BoundsTag);
	if (BoundPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("AGeoGameCamera: No AGeoTargetPoint with tag %s found — using default bounds ±500."),
			   *BoundsTag.ToString());
		return;
	}

	FBox2D Result(ForceInit);
	for (AActor const* Point : BoundPoints)
	{
		if (Point)
		{
			Result += FVector2D(Point->GetActorLocation());
		}
	}
	Bounds = Result;
}

void AGeoGameCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	FVector2D const ScreenRight = FVector2D(GetActorRightVector()).GetSafeNormal();
	FVector2D const ScreenUp = FVector2D(GetActorUpVector()).GetSafeNormal();

	float const ViewportHalfExtentX =
		OrthoHalfWidth * FMath::Abs(ScreenRight.X) + OrthoHalfHeight * FMath::Abs(ScreenUp.X);
	float const ViewportHalfExtentY =
		OrthoHalfWidth * FMath::Abs(ScreenRight.Y) + OrthoHalfHeight * FMath::Abs(ScreenUp.Y);

	float const MinCameraX = Bounds.Min.X + ViewportHalfExtentX;
	float const MaxCameraX = Bounds.Max.X - ViewportHalfExtentX;
	float const MinCameraY = Bounds.Min.Y + ViewportHalfExtentY;
	float const MaxCameraY = Bounds.Max.Y - ViewportHalfExtentY;

	FVector2D const PawnXY(LocalPawn->GetActorLocation());
	FVector2D const BoundsCenter = Bounds.GetCenter();
	FVector2D const ClampedTarget(
		MinCameraX <= MaxCameraX ? FMath::Clamp(PawnXY.X, MinCameraX, MaxCameraX) : BoundsCenter.X,
		MinCameraY <= MaxCameraY ? FMath::Clamp(PawnXY.Y, MinCameraY, MaxCameraY) : BoundsCenter.Y);

	FVector2D const CameraXY(GetActorLocation());
	FVector2D const NewXY = FMath::Vector2DInterpTo(CameraXY, ClampedTarget, DeltaTime, FollowInterpSpeed);

	FVector const CurrentLocation = GetActorLocation();
	SetActorLocation(FVector(NewXY.X, NewXY.Y, CurrentLocation.Z));
}
