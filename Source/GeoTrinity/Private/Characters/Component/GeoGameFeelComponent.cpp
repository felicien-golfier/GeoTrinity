// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/Component/GeoGameFeelComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoGameFeelComponent::UGeoGameFeelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGeoGameFeelComponent::BeginPlay()
{
	Super::BeginPlay();

	TargetMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (!TargetMesh)
	{
		TargetMesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	}

	if (TargetMesh)
	{
		InitialMeshRelativeLocation = TargetMesh->GetRelativeLocation();
	}
	else
	{
		UE_LOG(LogGeoTrinity, Warning,
			   TEXT("[GeoGameFeelComponent] No mesh found on %s — hit flash and recoil will be no-ops."),
			   *GetOwner()->GetName());
	}
}

void UGeoGameFeelComponent::TickComponent(float DeltaTime, ELevelTick TickType,
										  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!TargetMesh)
	{
		return;
	}

	if (CurrentRecoilOffset.IsNearlyZero(0.1f))
	{
		CurrentRecoilOffset = FVector::ZeroVector;
		TargetMesh->SetRelativeLocation(InitialMeshRelativeLocation);
		SetComponentTickEnabled(false);
		return;
	}

	CurrentRecoilOffset = FMath::VInterpTo(CurrentRecoilOffset, FVector::ZeroVector, DeltaTime, RecoilRecoverySpeed);
	TargetMesh->SetRelativeLocation(InitialMeshRelativeLocation + CurrentRecoilOffset);
}

void UGeoGameFeelComponent::FlashOnHit()
{
	if (!TargetMesh || HitFlashTimerHandle.IsValid())
	{
		return;
	}

	UGameDataSettings const* GDSettings = GetDefault<UGameDataSettings>();

	bool const bIsLocalPlayer = GeoLib::IsLocalPlayerAvatar(Cast<APawn>(GetOwner()));
	TSoftObjectPtr<UMaterialInterface> const& MaterialRef =
		bIsLocalPlayer ? GDSettings->LocalPlayerHitFlashMaterial : GDSettings->HitFlashMaterial;

	UMaterialInterface* FlashMaterial = MaterialRef.LoadSynchronous();
	if (!ensureMsgf(FlashMaterial, TEXT("Fill %s in your game data settings"),
					bIsLocalPlayer ? TEXT("LocalPlayerHitFlashMaterial") : TEXT("HitFlashMaterial")))
	{
		return;
	}

	TargetMesh->SetOverlayMaterial(FlashMaterial);
	// Weak-bound: the timer is dropped if the component is destroyed first.
	GetWorld()->GetTimerManager().SetTimer(HitFlashTimerHandle,
										   FTimerDelegate::CreateWeakLambda(this,
																			[this]()
																			{
																				TargetMesh->SetOverlayMaterial(nullptr);
																				HitFlashTimerHandle.Invalidate();
																			}),
										   GDSettings->HitFlashDuration, false);
}

void UGeoGameFeelComponent::ApplyRecoil(float Distance)
{
	if (!TargetMesh)
	{
		return;
	}

	CurrentRecoilOffset = FVector(-1, 0, 0.f) * Distance;
	TargetMesh->SetRelativeLocation(InitialMeshRelativeLocation + CurrentRecoilOffset);
	SetComponentTickEnabled(true);
}

bool UGeoGameFeelComponent::IsDamageCueAvailable()
{
	double const Now = GetWorld()->GetTimeSeconds();
	float const RateLimit = 1.f / GetDefault<UGameDataSettings>()->GameplayCueRateLimitPerSecond;
	if (Now - LastDamageCueTime < RateLimit)
	{
		return false;
	}
	LastDamageCueTime = Now;
	return true;
}

bool UGeoGameFeelComponent::IsHealCueAvailable()
{
	double const Now = GetWorld()->GetTimeSeconds();
	float const RateLimit = 1.f / GetDefault<UGameDataSettings>()->GameplayCueRateLimitPerSecond;
	if (Now - LastHealCueTime >= RateLimit)
	{
		LastHealCueTime = Now;
		return true;
	}

	return false;
}
