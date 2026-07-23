// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/BuffPickup/GeoBuffPickup.h"

#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemInterface.h"
#include "Characters/PlayableCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoBuffPickup::AGeoBuffPickup(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());

	BuffMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuffMeshComponent"));
	BuffMeshComponent->SetupAttachment(VisualRoot);

	bUseRegularDrain = false;
	bDestroyOldestWhenLimitReached = true;
	bSurviveOverTheVoid = true;
	// The reload buff shower relies on the manager's count limit to evict the oldest uncollected pickup.
	bUnlimitedDeploy = false;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoBuffPickup, Data, COND_InitialOnly);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::InitInteractable(FInteractableActorData* InputData)
{
	FBuffPickupData* BuffPickupData = static_cast<FBuffPickupData*>(InputData);
	ensureMsgf(BuffPickupData, TEXT("AGeoBuffPickup: InputData is not a FBuffPickupData!"));
	if (!BuffPickupData)
	{
		return;
	}


	Data = *BuffPickupData;
	StartTime = GetWorld()->GetTimeSeconds();
	Super::InitInteractable(InputData);
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
	UpdateColor();
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
	UpdateColor();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::UpdateColor()
{
	if (Data.BuffIndex < 0)
	{
		return;
	}

	UGeoReloadAbility const* ReloadAbilityCDO = GeoASLib::GetAbilityCDO<UGeoReloadAbility>(Data.AbilityTag);
	if (!ensureMsgf(ReloadAbilityCDO, TEXT("AGeoBuffPickup: no UGeoReloadAbility CDO for AbilityTag %s"),
					*Data.AbilityTag.ToString()))
	{
		return;
	}

	if (!BuffMaterialInstance)
	{
		BuffMaterialInstance = BuffMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!ensureMsgf(BuffMaterialInstance, TEXT("AGeoBuffPickup: BuffMeshComponent has no material to tint")))
	{
		return;
	}

	BuffMaterialInstance->SetVectorParameterValue(ColorParameterName,
												  ReloadAbilityCDO->GetColorForIndex(Data.BuffIndex));
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::OnOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool,
							   FHitResult const&)
{
	bool bHasElapsedTimeBeforePickup = GetWorld()->GetTimeSeconds() >= StartTime + TimeBeforePickup;
	if (!GeoLib::IsServer(GetWorld()) || !bHasElapsedTimeBeforePickup || !IsValid(OtherActor)
		|| !OtherActor->CanBeDamaged())
	{
		return;
	}

	IAbilitySystemInterface const* OtherActorAbilitySystem = Cast<IAbilitySystemInterface>(OtherActor);
	if (!Cast<APlayableCharacter>(OtherActor) || !OtherActorAbilitySystem)
	{
		return;
	}

	if (!GeoASLib::IsTeamAttitudeAligned(GetData()->Owner, OtherActor, OverlapAttitude))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = OtherActorAbilitySystem->GetAbilitySystemComponent();
	UAbilitySystemComponent* OwnerASC = GeoASLib::GetGeoAscFromActor(GetData()->Owner);
	if (!TargetASC || !OwnerASC)
	{
		return;
	}

	UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(Data.EffectDataArray, OwnerASC, TargetASC, Data.Level,
														Data.Seed, Data.AbilityTag);

	Recall();
}
