// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Characters/GeoCharacter.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Input/GeoInputComponent.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/GeoCameraVolume.h"

AGeoGameCamera::AGeoGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGeoGameCamera::EnterVolume(AGeoCameraVolume* Volume)
{
	ActiveVolumes.AddUnique(Volume);
	RefreshBounds();
}

void AGeoGameCamera::ExitVolume(AGeoCameraVolume* Volume)
{
	ActiveVolumes.Remove(Volume);
	RefreshBounds();
}

AGeoCameraVolume* AGeoGameCamera::GetActiveVolume()
{
	for (int32 Index = ActiveVolumes.Num() - 1; Index >= 0; --Index)
	{
		if (AGeoCameraVolume* Volume = ActiveVolumes[Index].Get())
		{
			return Volume;
		}
		ActiveVolumes.RemoveAt(Index);
	}
	return nullptr;
}

void AGeoGameCamera::RefreshBounds()
{
	AGeoCameraVolume* Volume = GetActiveVolume();
	if (!Volume)
	{
		bBounded = false;
		return;
	}

	TArray<AActor*> const BoundPoints =
		GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_CameraBounds, Volume->GetArenaTag());
	if (BoundPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			   TEXT("AGeoGameCamera: no TargetPoint.CameraBounds for arena %s — camera unbounded in that volume."),
			   *Volume->GetArenaTag().ToString());
		bBounded = false;
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
	bBounded = true;
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

	FVector2D TargetXY(LocalPawn->GetActorLocation());
	if (bSpectating)
	{
		SpectateTarget += GetSpectateMoveInput(LocalPlayerController, LocalCharacter) * SpectateMoveSpeed * DeltaTime;
		TargetXY = SpectateTarget;
	}

	FVector2D FollowTarget = TargetXY;
	if (bBounded)
	{
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

		FVector2D const BoundsCenter = Bounds.GetCenter();
		FollowTarget =
			FVector2D(MinCameraX <= MaxCameraX ? FMath::Clamp(TargetXY.X, MinCameraX, MaxCameraX) : BoundsCenter.X,
					  MinCameraY <= MaxCameraY ? FMath::Clamp(TargetXY.Y, MinCameraY, MaxCameraY) : BoundsCenter.Y);
	}
	if (bSpectating)
	{
		SpectateTarget = FollowTarget;
	}

	FVector2D const CameraXY(GetActorLocation());
	FVector2D const NewXY = FMath::Vector2DInterpTo(CameraXY, FollowTarget, DeltaTime, FollowInterpSpeed);

	FVector const CurrentLocation = GetActorLocation();
	SetActorLocation(FVector(NewXY.X, NewXY.Y, CurrentLocation.Z));
}
