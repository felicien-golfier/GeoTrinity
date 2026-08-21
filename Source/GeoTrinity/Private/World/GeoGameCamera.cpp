// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoGameCamera.h"

#include "Camera/CameraComponent.h"
#include "Characters/GeoCharacter.h"
#include "Containers/ArrayView.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/PlayerController.h"
#include "Input/GeoInputComponent.h"
#include "InputActionValue.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Settings/GameDataSettings.h"
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

	FVector2D Average(TConstArrayView<FVector2D> Points)
	{
		FVector2D Sum = FVector2D::ZeroVector;
		for (FVector2D const& Point : Points)
		{
			Sum += Point;
		}
		return Sum / Points.Num();
	}
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
	TargetZoom = BaseOrthoWidth;
	RefreshVolumeZoom();
	CurrentOrthoWidth = TargetZoom;
	ApplyOutlineMaterial();
	ensureMsgf(CameraParameters,
			   TEXT("AGeoGameCamera: no CameraParameters — backdrop layers stay welded to the floor"));
	ensureMsgf(GetCameraComponent()->bConstrainAspectRatio,
			   TEXT("AGeoGameCamera: unconstrained aspect ratio — the view is then shaped by the window instead of "
					"the camera's AspectRatio, and the volume clamp insets by the wrong height"));
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
	RefreshVolumeZoom();
}

void AGeoGameCamera::ExitVolume(AGeoCameraVolume* Volume)
{
	ActiveVolumes.RemoveSingle(Volume);
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

void AGeoGameCamera::RefreshVolumeZoom()
{
	if (AGeoCameraVolume const* const Volume = GetActiveVolume())
	{
		TargetZoom = Volume->GetOrthoWidth();
	}
}

void AGeoGameCamera::AddZoomInput(float WheelValue)
{
	TargetZoom -= WheelValue * ZoomWheelStep;
}

FInputActionValue AGeoGameCamera::ReadInputValue(APlayerController const* PlayerController,
												 UInputAction const* Action) const
{
	ULocalPlayer const* const LocalPlayer = PlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem const* const InputSubsystem =
		LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	if (!Action || !InputSubsystem)
	{
		return FInputActionValue();
	}
	return InputSubsystem->GetPlayerInput()->GetActionValue(Action);
}

void AGeoGameCamera::GatherLocalPlayers(TArray<FVector2D, TInlineAllocator<4>>& OutLivingPlayers,
										APlayerController const*& OutFirstController,
										UGeoInputComponent const*& OutFirstInputComponent) const
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController const* PlayerController = Iterator->Get();
		APawn const* Pawn =
			PlayerController && PlayerController->IsLocalController() ? PlayerController->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		AGeoCharacter const* Character = Cast<AGeoCharacter>(Pawn);
		if (!OutFirstController)
		{
			OutFirstController = PlayerController;
			OutFirstInputComponent = Character ? Character->GetGeoInputComponent() : nullptr;
		}
		// A reviving player is framed like a living one: they have already been moved to where they come back, and
		// spectating through their own revive would leave the camera behind on the corpse's last room.
		if (!Character || !Character->IsDead() || Character->IsReviving())
		{
			OutLivingPlayers.Add(FVector2D(Pawn->GetActorLocation()));
		}
	}
}

void AGeoGameCamera::UpdateSpectateState(bool bAllLocalPlayerDead, FVector2D CameraXY,
										 APlayerController const* FirstLocalController,
										 UGeoInputComponent const* FirstLocalInputComponent, float DeltaTime)
{
	if (bAllLocalPlayerDead && !bSpectating)
	{
		SpectateTarget = CameraXY;
		AGeoGameState const* GameState = GetWorld()->GetGameState<AGeoGameState>();
		SpectateDelayRemaining = GameState ? GameState->DeathTime : 0.f;
	}
	bSpectating = bAllLocalPlayerDead;
	if (!bSpectating)
	{
		return;
	}

	SpectateDelayRemaining = FMath::Max(SpectateDelayRemaining - DeltaTime, 0.f);
	if (SpectateDelayRemaining <= 0.f && FirstLocalInputComponent)
	{
		SpectateTarget += ReadInputValue(FirstLocalController, FirstLocalInputComponent->MoveAction).Get<FVector2D>()
			* SpectateMoveSpeed * DeltaTime;
	}
}

void AGeoGameCamera::UpdateZoom(TConstArrayView<FVector2D> LivingPlayers, FVector2D CameraXY, float DeltaTime)
{
	UGameDataSettings const* const Settings = GetDefault<UGameDataSettings>();
	TargetZoom = FMath::Clamp(TargetZoom, Settings->MinOrthoWidth, Settings->MaxOrthoWidth);

	float FarthestDistance = 0.f;
	if (LivingPlayers.Num() > 1)
	{
		for (FVector2D const& PlayerXY : LivingPlayers)
		{
			FarthestDistance =
				FMath::Max(FarthestDistance, static_cast<float>(FVector2D::Distance(PlayerXY, CameraXY)));
		}
	}
	float const DesiredOrthoWidth = FMath::GetMappedRangeValueClamped(
		FVector2f(ZoomMinDistance, ZoomMaxDistance), FVector2f(TargetZoom, Settings->MaxOrthoWidth), FarthestDistance);
	CurrentOrthoWidth = FMath::FInterpTo(CurrentOrthoWidth, DesiredOrthoWidth, DeltaTime, ZoomInterpSpeed);
	GetCameraComponent()->SetOrthoWidth(CurrentOrthoWidth);

	if (CVarShowCameraZoom.GetValueOnGameThread())
	{
		DrawZoomDebug(LivingPlayers, CameraXY, FarthestDistance);
	}
}

void AGeoGameCamera::DrawZoomDebug(TConstArrayView<FVector2D> LivingPlayers, FVector2D CameraXY,
								   float FarthestDistance) const
{
	for (FVector2D const& PlayerXY : LivingPlayers)
	{
		DrawDebugLine(GetWorld(), FVector(PlayerXY, ArbitraryCharacterZ), FVector(CameraXY, ArbitraryCharacterZ),
					  FColor::Yellow, false, 0.f, 0, 3.f);
	}
	GEngine->AddOnScreenDebugMessage(
		-1, 0.f, FColor::Yellow,
		FString::Printf(TEXT("Camera zoom: farthest %.0f (min %.0f / max %.0f) | Zoom %.0f | OrthoWidth %.0f"),
						FarthestDistance, ZoomMinDistance, ZoomMaxDistance, TargetZoom, CurrentOrthoWidth));
}

FVector2D AGeoGameCamera::ClampToVolume(FVector2D Target, AGeoCameraVolume const* Volume) const
{
	FBox2D const Bounds = Volume ? Volume->GetViewBounds() : FBox2D(ForceInit);
	if (!Bounds.bIsValid)
	{
		return Target;
	}

	UCameraComponent const* const Camera = GetCameraComponent();
	FVector2D const ScreenRight = FVector2D(Camera->GetRightVector()).GetSafeNormal();
	FVector2D const ScreenUp = FVector2D(Camera->GetUpVector()).GetSafeNormal();

	float const OrthoHalfWidth = CurrentOrthoWidth * 0.5f;
	float const OrthoHalfHeight = OrthoHalfWidth / Camera->AspectRatio;

	float const ViewportHalfExtentX =
		OrthoHalfWidth * FMath::Abs(ScreenRight.X) + OrthoHalfHeight * FMath::Abs(ScreenUp.X);
	float const ViewportHalfExtentY =
		OrthoHalfWidth * FMath::Abs(ScreenRight.Y) + OrthoHalfHeight * FMath::Abs(ScreenUp.Y);

	float const MinCameraX = Bounds.Min.X + ViewportHalfExtentX;
	float const MaxCameraX = Bounds.Max.X - ViewportHalfExtentX;
	float const MinCameraY = Bounds.Min.Y + ViewportHalfExtentY;
	float const MaxCameraY = Bounds.Max.Y - ViewportHalfExtentY;

	FVector2D const BoundsCenter = Bounds.GetCenter();
	return FVector2D(MinCameraX <= MaxCameraX ? FMath::Clamp(Target.X, MinCameraX, MaxCameraX) : BoundsCenter.X,
					 MinCameraY <= MaxCameraY ? FMath::Clamp(Target.Y, MinCameraY, MaxCameraY) : BoundsCenter.Y);
}

void AGeoGameCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<FVector2D, TInlineAllocator<4>> LivingPlayers;
	APlayerController const* FirstLocalController = nullptr;
	UGeoInputComponent const* FirstLocalInputComponent = nullptr;
	GatherLocalPlayers(LivingPlayers, FirstLocalController, FirstLocalInputComponent);
	if (!FirstLocalController)
	{
		return;
	}

	AGeoCameraVolume const* const ActiveVolume = GetActiveVolume();
	FVector2D const CameraXY(GetActorLocation());
	UpdateSpectateState(LivingPlayers.IsEmpty(), CameraXY, FirstLocalController, FirstLocalInputComponent, DeltaTime);
	UpdateZoom(LivingPlayers, CameraXY, DeltaTime);

	FVector2D const FollowTarget = ClampToVolume(bSpectating ? SpectateTarget : Average(LivingPlayers), ActiveVolume);
	if (bSpectating)
	{
		SpectateTarget = FollowTarget;
	}

	FVector2D const NewXY = FMath::Vector2DInterpTo(CameraXY, FollowTarget, DeltaTime, FollowInterpSpeed);
	SetActorLocation(FVector(NewXY.X, NewXY.Y, GetActorLocation().Z));
	PublishCameraParameters(NewXY);
}
