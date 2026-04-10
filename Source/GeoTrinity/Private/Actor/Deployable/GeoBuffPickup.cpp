// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoBuffPickup.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemInterface.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoBuffPickup::AGeoBuffPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());

	BuffMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuffMeshComponent"));
	BuffMeshComponent->SetupAttachment(VisualRoot);

	bUseRegularDrain = false;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoBuffPickup, Data, COND_InitialOnly);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::InitInteractableData(FInteractableActorData* InputData)
{
	FBuffPickupData* BuffPickupData = static_cast<FBuffPickupData*>(InputData);
	ensureMsgf(BuffPickupData, TEXT("AGeoBuffPickup: InputData is not a FBuffPickupData!"));
	if (!BuffPickupData)
	{
		return;
	}

	Data = *BuffPickupData;

	Super::InitInteractableData(InputData);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::BeginPlay()
{
	Super::BeginPlay();

	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlap);

	float const Scale = FMath::Lerp(MinScale, MaxScale, FMath::Clamp(Data.PowerScale, 0.f, 1.f));
	SetActorScale3D(FVector(Scale));

	ensureMsgf(LaunchCurve, TEXT("AGeoBuffPickup: LaunchCurve is not set — pickup will snap to target."));
	if (LaunchCurve)
	{
		LaunchStartLocation = GetActorLocation();
	}
	else
	{
		bMovingToTarget = false;
		SetActorLocation(Data.TargetLocation);
	}
	UpdateMesh();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	VisualRoot->AddLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));

	if (bMovingToTarget)
	{
		LaunchElapsedTime += DeltaTime;

		float MinTime, MaxTime;
		LaunchCurve->GetTimeRange(MinTime, MaxTime);
		ensureMsgf(MinTime == 0.f, TEXT("Min time must start at 0"));

		if (LaunchElapsedTime >= MaxTime)
		{
			LaunchElapsedTime = MaxTime;
			bMovingToTarget = false;
		}

		float const Alpha = LaunchCurve->GetFloatValue(LaunchElapsedTime);
		SetActorLocation(FMath::Lerp(LaunchStartLocation, Data.TargetLocation, Alpha));
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::OnRep_Data()
{
	UpdateMesh();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::UpdateMesh()
{
	ensureMsgf(Data.MeshIndex < 0 || BuffMeshAssets.IsValidIndex(Data.MeshIndex),
			   TEXT("AGeoBuffPickup: MeshIndex %d is out of range — BuffMeshAssets has %d entries. "
					"BuffMeshAssets and BuffEffectDataAssets must have the same number of entries."),
			   Data.MeshIndex, BuffMeshAssets.Num());

	if (BuffMeshAssets.IsValidIndex(Data.MeshIndex))
	{
		BuffMeshComponent->SetStaticMesh(BuffMeshAssets[Data.MeshIndex]);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::OnOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool,
							   FHitResult const&)
{
	if (!GeoLib::IsServer(GetWorld()) || bMovingToTarget || !IsValid(OtherActor))
	{
		return;
	}

	if (Cast<AGeoBuffPickup>(OtherActor))
	{
		return;
	}

	IAbilitySystemInterface const* OtherActorAbilitySystem = Cast<IAbilitySystemInterface>(OtherActor);
	if (!OtherActorAbilitySystem)
	{
		return;
	}

	if (!GeoASLib::IsTeamAttitudeAligned(GetData()->CharacterOwner, OtherActor, OverlapAttitude))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = OtherActorAbilitySystem->GetAbilitySystemComponent();
	UAbilitySystemComponent* OwnerASC = GeoASLib::GetGeoAscFromActor(GetData()->CharacterOwner);
	if (!TargetASC || !OwnerASC)
	{
		return;
	}

	UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(Data.EffectDataArray, OwnerASC, TargetASC, Data.Level,
														Data.Seed);

	OnRecalled();
}
