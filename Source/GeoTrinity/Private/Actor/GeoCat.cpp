// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoCat.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoCat::AGeoCat()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
}

void AGeoCat::BeginPlay()
{
	Super::BeginPlay();
	IdleVariantCountdown = FMath::FRandRange(IdleVariantPause.X, IdleVariantPause.Y);
}

void AGeoCat::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsHidden())
	{
		AdvanceMove(DeltaSeconds);
	}
}

void AGeoCat::Walk(TArray<FVector> const& Points, float const Speed, float const StopDistance)
{
	BeginOrder(EGeoCatOrder::Walk);
	WalkSpeed = Speed;
	for (FVector const& Point : Points)
	{
		Waypoints.Add(FVector(Point.X, Point.Y, GetActorLocation().Z));
	}

	if (!ensureMsgf(!IsHidden(), TEXT("%s: Walk while hidden, a hidden cat never moves: the walk ends at once"),
					*GetName()))
	{
		EndOrder(true);
	}
	else if (Waypoints.IsEmpty())
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("%s: Walk with no point"), *GetName());
		EndOrder(true);
	}
	else if (StopDistance > 0.f)
	{
		FVector const Last = Waypoints.Last();
		FVector const From = Waypoints.Num() > 1 ? Waypoints[Waypoints.Num() - 2] : GetActorLocation();
		Waypoints.Last() = Last + (From - Last).GetSafeNormal2D() * StopDistance;
		FaceAtEnd = Last;
	}
}

void AGeoCat::Roam(FGeoCatRoam const& Settings)
{
	BeginOrder(EGeoCatOrder::Roam);
	RoamSettings = Settings;
	WalkSpeed = Settings.Speed;
	WalkToRandomRoamSpot();
}

void AGeoCat::PlayMontage(UAnimMontage* Montage)
{
	BeginOrder(EGeoCatOrder::Montage);
	UAnimInstance* const AnimInstance = Mesh->GetAnimInstance();
	if (Montage && AnimInstance && AnimInstance->Montage_Play(Montage) > 0.f)
	{
		FOnMontageBlendingOutStarted BlendingOutDelegate =
			FOnMontageBlendingOutStarted::CreateUObject(this, &AGeoCat::OnMontageBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);
	}
	else
	{
		if (Mesh->GetSkeletalMeshAsset())
		{
			UE_LOG(LogGeoTrinity, Warning, TEXT("%s: montage %s could not play, the order completes at once"),
				   *GetName(), *GetNameSafe(Montage));
		}

		EndOrder(true);
	}
}

void AGeoCat::AdvanceMove(float const DeltaSeconds)
{
	bool const bMoving = !Waypoints.IsEmpty();
	bool const bMontageOrderRunning = bOrderRunning && Order == EGeoCatOrder::Montage;
	UAnimInstance const* const AnimInstance = Mesh->GetAnimInstance();
	bool const bMontagePlaying = AnimInstance && AnimInstance->IsAnyMontagePlaying();
	if (bMoving)
	{
		FVector const Location = GetActorLocation();
		FVector const ToWaypoint = Waypoints[0] - Location;
		float const Step = WalkSpeed * DeltaSeconds;
		TurnToward(ToWaypoint, DeltaSeconds);
		if (ToWaypoint.Size2D() <= Step)
		{
			SetActorLocation(Waypoints[0]);
			Waypoints.RemoveAt(0);
			if (Waypoints.IsEmpty())
			{
				OnWalkEnded();
			}
		}
		else
		{
			SetActorLocation(Location + ToWaypoint.GetSafeNormal2D() * Step);
		}
	}
	else if (APawn const* const Player =
				 bFacePlayer && !bMontagePlaying ? UGameplayStatics::GetPlayerPawn(this, 0) : nullptr)
	{
		TurnToward(Player->GetActorLocation() - GetActorLocation(), DeltaSeconds);
	}

	AnimSpeed = FMath::FInterpConstantTo(AnimSpeed, bMoving ? WalkSpeed : 0.f, DeltaSeconds, SpeedBlendRate);
	WriteAnimSpeed();

	if (!bMoving && !bMontageOrderRunning && !bMontagePlaying)
	{
		TickIdleVariant(DeltaSeconds);
	}
}

UAnimMontage* AGeoCat::PickIdleVariant()
{
	return GeoLib::DrawFromShuffledBag(IdleVariantBag, IdleVariants, LastIdleVariant);
}

void AGeoCat::Stop()
{
	EndOrder(false);
	StopMontages();
}

void AGeoCat::BeginOrder(EGeoCatOrder const NewOrder)
{
	Stop();
	Order = NewOrder;
	bOrderRunning = true;
}

void AGeoCat::EndOrder(bool const bCompleted)
{
	if (bOrderRunning)
	{
		bOrderRunning = false;
		Waypoints.Reset();
		FaceAtEnd.Reset();
		GetWorldTimerManager().ClearTimer(RoamPauseTimer);
		OnOrderEnded.Broadcast(bCompleted);
	}
}

void AGeoCat::OnWalkEnded()
{
	if (FaceAtEnd.IsSet())
	{
		SetActorRotation(FRotator(0.f, (FaceAtEnd.GetValue() - GetActorLocation()).Rotation().Yaw, 0.f));
	}

	if (Order == EGeoCatOrder::Roam)
	{
		GetWorldTimerManager().SetTimer(RoamPauseTimer, this, &AGeoCat::WalkToRandomRoamSpot,
										FMath::FRandRange(RoamSettings.Pause.X, RoamSettings.Pause.Y));
	}
	else
	{
		EndOrder(true);
	}
}

void AGeoCat::WalkToRandomRoamSpot()
{
	StopMontages();
	if (!RoamSettings.Area)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("%s: Roam with no Area, standing still"), *GetName());
	}
	else
	{
		FVector Origin;
		FVector Extent;
		RoamSettings.Area->GetActorBounds(false, Origin, Extent);
		float const HalfX = FMath::Max(Extent.X - RoamSettings.Margin, 0.f);
		float const HalfY = FMath::Max(Extent.Y - RoamSettings.Margin, 0.f);
		Waypoints.Add(FVector(Origin.X + FMath::FRandRange(-HalfX, HalfX), Origin.Y + FMath::FRandRange(-HalfY, HalfY),
							  GetActorLocation().Z));
	}
}

void AGeoCat::PlayIdleVariant()
{
	UAnimInstance* const AnimInstance = Mesh->GetAnimInstance();
	UAnimMontage* const Variant = PickIdleVariant();
	if (Variant && AnimInstance)
	{
		AnimInstance->Montage_Play(Variant);
	}
}

void AGeoCat::StopMontages() const
{
	if (UAnimInstance* const AnimInstance = Mesh->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.2f);
	}
}

void AGeoCat::TickIdleVariant(float const DeltaSeconds)
{
	IdleVariantCountdown -= DeltaSeconds;
	if (IdleVariantCountdown <= 0.f)
	{
		PlayIdleVariant();
		IdleVariantCountdown = FMath::FRandRange(IdleVariantPause.X, IdleVariantPause.Y);
	}
}

void AGeoCat::OnMontageBlendingOut(UAnimMontage* /*Montage*/, bool const bInterrupted)
{
	if (!bInterrupted && bOrderRunning && Order == EGeoCatOrder::Montage)
	{
		EndOrder(true);
	}
}

void AGeoCat::TurnToward(FVector const& Direction, float const DeltaSeconds)
{
	if (Direction.SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
	{
		float const Yaw = FMath::FixedTurn(GetActorRotation().Yaw, Direction.Rotation().Yaw, TurnRate * DeltaSeconds);
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}
}

void AGeoCat::WriteAnimSpeed() const
{
	UAnimInstance* const AnimInstance = Mesh->GetAnimInstance();
	if (AnimInstance)
	{
		FName const SpeedVariable(TEXT("Speed"));
		if (FDoubleProperty const* const DoubleProperty =
				FindFProperty<FDoubleProperty>(AnimInstance->GetClass(), SpeedVariable))
		{
			DoubleProperty->SetPropertyValue_InContainer(AnimInstance, AnimSpeed);
		}
		else if (FFloatProperty const* const FloatProperty =
					 FindFProperty<FFloatProperty>(AnimInstance->GetClass(), SpeedVariable))
		{
			FloatProperty->SetPropertyValue_InContainer(AnimInstance, AnimSpeed);
		}
	}
}
