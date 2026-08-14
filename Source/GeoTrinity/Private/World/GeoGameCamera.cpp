// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Characters/GeoCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Input/GeoInputComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tool/GeoColor.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "World/GeoBackdropComponent.h"
#include "World/GeoBackgroundPulseComponent.h"
#include "World/GeoCameraVolume.h"

static TAutoConsoleVariable CVarShowCameraZoom(
	TEXT("Geo.ShowCameraZoom"), false,
	TEXT("When true, draws a line from every living local player to the camera and prints the zoom distance"));

namespace
{
	FName const CameraXYParam(TEXT("CameraXY"));
	FName const ZoomRatioParam(TEXT("ZoomRatio"));
} // namespace

AGeoGameCamera::AGeoGameCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	BackgroundPulse = CreateDefaultSubobject<UGeoBackgroundPulseComponent>(TEXT("BackgroundPulse"));
	Backdrop = CreateDefaultSubobject<UGeoBackdropComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(GetCameraComponent());
}

void AGeoGameCamera::BeginPlay()
{
	Super::BeginPlay();
	BaseOrthoWidth = GetCameraComponent()->OrthoWidth;
	CurrentOrthoWidth = BaseOrthoWidth;
	ApplyOutlineMaterial();
	ensureMsgf(CameraParameters, TEXT("AGeoGameCamera: no CameraParameters — backdrop layers stay welded to the floor"));
}

void AGeoGameCamera::PublishCameraParameters(FVector2D CameraXY)
{
	if (!CameraParameters)
	{
		return;
	}

	UKismetMaterialLibrary::SetVectorParameterValue(this, CameraParameters, CameraXYParam,
												   FLinearColor(CameraXY.X, CameraXY.Y, 0.f));
	UKismetMaterialLibrary::SetScalarParameterValue(this, CameraParameters, ZoomRatioParam,
													BaseOrthoWidth / CurrentOrthoWidth);
}

void AGeoGameCamera::ApplyOutlineMaterial()
{
	if (GeoLib::IsDedicatedServer(this)
		|| !ensureMsgf(OutlineMaterial,
					   TEXT("AGeoGameCamera: no OutlineMaterial — deployables render without an outline")))
	{
		return;
	}

	UMaterialInstanceDynamic* const PaletteMaterial = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
	PaletteMaterial->SetTextureParameterValue(GeoColor::PaletteTextureParam, GeoColor::CreatePaletteTexture());
	PaletteMaterial->SetScalarParameterValue(GeoColor::PaletteSizeParam, static_cast<float>(GeoColor::SlotCount));
	GetCameraComponent()->PostProcessSettings.AddBlendable(PaletteMaterial, 1.f);
}

void AGeoGameCamera::EnterVolume(AGeoCameraVolume* Volume)
{
	ActiveVolumes.Add(Volume);
	RefreshBounds();
}

void AGeoGameCamera::ExitVolume(AGeoCameraVolume* Volume)
{
	ActiveVolumes.RemoveSingle(Volume);
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

	APlayerController* FirstLocalController = nullptr;
	AGeoCharacter const* FirstLocalCharacter = nullptr;
	TArray<FVector2D, TInlineAllocator<4>> LivingPlayers;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		APawn const* Pawn =
			PlayerController && PlayerController->IsLocalController() ? PlayerController->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		AGeoCharacter const* Character = Cast<AGeoCharacter>(Pawn);
		if (!FirstLocalController)
		{
			FirstLocalController = PlayerController;
			FirstLocalCharacter = Character;
		}
		if (!Character || !Character->IsDead())
		{
			LivingPlayers.Add(FVector2D(Pawn->GetActorLocation()));
		}
	}
	if (!FirstLocalController)
	{
		return;
	}

	FVector2D const CameraXY(GetActorLocation());

	bool const bAllLocalPlayerDead = LivingPlayers.IsEmpty();
	if (bAllLocalPlayerDead && !bSpectating)
	{
		SpectateTarget = CameraXY;
		AGeoGameState const* GameState = GetWorld()->GetGameState<AGeoGameState>();
		SpectateDelayRemaining = GameState ? GameState->DeathTime : 0.f;
	}
	bSpectating = bAllLocalPlayerDead;

	bool bSpectatePanAllowed = false;
	if (bSpectating)
	{
		SpectateDelayRemaining = FMath::Max(SpectateDelayRemaining - DeltaTime, 0.f);
		bSpectatePanAllowed = SpectateDelayRemaining <= 0.f;
	}

	FVector2D TargetXY = FVector2D::ZeroVector;
	if (bSpectatePanAllowed)
	{
		SpectateTarget +=
			GetSpectateMoveInput(FirstLocalController, FirstLocalCharacter) * SpectateMoveSpeed * DeltaTime;
		TargetXY = SpectateTarget;
	}
	else if (bSpectating)
	{
		TargetXY = SpectateTarget;
	}
	else
	{
		for (FVector2D const& PlayerXY : LivingPlayers)
		{
			TargetXY += PlayerXY;
		}
		TargetXY /= LivingPlayers.Num();
	}

	float FarthestDistance = 0.f;
	for (FVector2D const& PlayerXY : LivingPlayers)
	{
		FarthestDistance = FMath::Max(FarthestDistance, static_cast<float>(FVector2D::Distance(PlayerXY, CameraXY)));
	}
	float const DesiredOrthoWidth = FMath::GetMappedRangeValueClamped(
		FVector2f(ZoomMinDistance, ZoomMaxDistance), FVector2f(BaseOrthoWidth, MaxOrthoWidth), FarthestDistance);
	CurrentOrthoWidth = FMath::FInterpTo(CurrentOrthoWidth, DesiredOrthoWidth, DeltaTime, ZoomInterpSpeed);
	GetCameraComponent()->SetOrthoWidth(CurrentOrthoWidth);

	if (CVarShowCameraZoom.GetValueOnGameThread())
	{
		for (FVector2D const& PlayerXY : LivingPlayers)
		{
			DrawDebugLine(GetWorld(), FVector(PlayerXY, ArbitraryCharacterZ), FVector(CameraXY, ArbitraryCharacterZ),
						  FColor::Yellow, false, 0.f, 0, 3.f);
		}
		GEngine->AddOnScreenDebugMessage(
			-1, 0.f, FColor::Yellow,
			FString::Printf(TEXT("Camera zoom: farthest %.0f (min %.0f / max %.0f) | OrthoWidth %.0f"),
							FarthestDistance, ZoomMinDistance, ZoomMaxDistance, CurrentOrthoWidth));
	}

	FVector2D FollowTarget = TargetXY;
	if (bBounded)
	{
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
		FVector2D const ScreenRight = FVector2D(GetActorRightVector()).GetSafeNormal();
		FVector2D const ScreenUp = FVector2D(GetActorUpVector()).GetSafeNormal();

		float const OrthoHalfWidth = CurrentOrthoWidth * 0.5f;
		float const OrthoHalfHeight = OrthoHalfWidth / AspectRatio;

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

	FVector2D const NewXY = FMath::Vector2DInterpTo(CameraXY, FollowTarget, DeltaTime, FollowInterpSpeed);

	FVector const CurrentLocation = GetActorLocation();
	SetActorLocation(FVector(NewXY.X, NewXY.Y, CurrentLocation.Z));
	PublishCameraParameters(NewXY);
}
