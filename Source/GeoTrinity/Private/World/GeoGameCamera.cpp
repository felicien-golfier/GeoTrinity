// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Characters/GeoCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerController.h"
#include "Input/GeoInputComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoGameCamera::AGeoGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGeoGameCamera::BeginPlay()
{
	Super::BeginPlay();

	TryBindToGameState();
}

void AGeoGameCamera::TryBindToGameState()
{
	AGeoGameState* GameState = GetWorld()->GetGameState<AGeoGameState>();
	if (!GameState)
	{
		// Late joiner: the replicated GameState has not arrived yet. Retry next tick rather than dereferencing null.
		GetWorldTimerManager().SetTimerForNextTick(this, &AGeoGameCamera::TryBindToGameState);
		return;
	}

	GameState->OnActiveArenaChanged.AddUniqueDynamic(this, &AGeoGameCamera::CalculateBounds);
	CalculateBounds();
}

void AGeoGameCamera::CalculateBounds()
{
	AGeoGameState const* GameState = GetWorld()->GetGameState<AGeoGameState>();
	if (!ensureMsgf(GameState, TEXT("AGeoGameCamera: bounds recalculated with no AGeoGameState")))
	{
		return;
	}

	SetArenaTag(GameState->GetActiveArenaTag());
}

void AGeoGameCamera::SetArenaTag(FGameplayTag ArenaTag)
{
	TArray<AActor*> BoundPoints =
		UGeoGameplayLibrary::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_CameraBounds, ArenaTag);
	if (BoundPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("AGeoGameCamera: No camera-bounds AGeoTargetPoint for arena %s — keeping previous bounds."),
			   *ArenaTag.ToString());
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

FVector2D AGeoGameCamera::GetSpectateMoveInput(APlayerController const* PlayerController,
											   AGeoCharacter const* LocalCharacter) const
{
	UInputAction const* MoveAction = LocalCharacter->GetGeoInputComponent()->MoveAction;
	ULocalPlayer const* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!MoveAction || !LocalPlayer)
	{
		return FVector2D::ZeroVector;
	}

	UEnhancedInputLocalPlayerSubsystem const* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!InputSubsystem)
	{
		return FVector2D::ZeroVector;
	}
	return InputSubsystem->GetPlayerInput()->GetActionValue(MoveAction).Get<FVector2D>();
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

	AGeoCharacter const* LocalCharacter = Cast<AGeoCharacter>(LocalPawn);
	bool const bDead = LocalCharacter && LocalCharacter->IsDead();
	if (bDead && !bSpectating)
	{
		SpectateTarget = FVector2D(GetActorLocation());
	}
	bSpectating = bDead;

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

	FVector2D TargetXY(LocalPawn->GetActorLocation());
	if (bSpectating)
	{
		SpectateTarget += GetSpectateMoveInput(LocalPlayerController, LocalCharacter) * SpectateMoveSpeed * DeltaTime;
		TargetXY = SpectateTarget;
	}

	FVector2D const BoundsCenter = Bounds.GetCenter();
	FVector2D const ClampedTarget(
		MinCameraX <= MaxCameraX ? FMath::Clamp(TargetXY.X, MinCameraX, MaxCameraX) : BoundsCenter.X,
		MinCameraY <= MaxCameraY ? FMath::Clamp(TargetXY.Y, MinCameraY, MaxCameraY) : BoundsCenter.Y);
	if (bSpectating)
	{
		SpectateTarget = ClampedTarget;
	}

	FVector2D const CameraXY(GetActorLocation());
	FVector2D const NewXY = FMath::Vector2DInterpTo(CameraXY, ClampedTarget, DeltaTime, FollowInterpSpeed);

	FVector const CurrentLocation = GetActorLocation();
	SetActorLocation(FVector(NewXY.X, NewXY.Y, CurrentLocation.Z));
}
