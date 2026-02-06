#include "Actor/Pickup/GeoBuffPickup.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/SphereComponent.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoBuffPickup::AGeoBuffPickup()
{
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(50.f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::Init()
{
	Super::Init();
	EffectDataArray.Empty();
	PowerScale = 1.f;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::End()
{
	Super::End();
	EffectDataArray.Empty();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::Setup(TArray<TInstancedStruct<FEffectData>> const& InEffectData, float InPowerScale)
{
	EffectDataArray = InEffectData;
	PowerScale = InPowerScale;

	const float Scale = FMath::Lerp(0.5f, 1.5f, FMath::Clamp(PowerScale, 0.f, 1.f));
	SetActorScale3D(FVector(Scale));
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoBuffPickup::OnSphereOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool,
									 FHitResult const&)
{
	if (!HasAuthority() || !IsValid(OtherActor))
	{
		return;
	}

	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OtherActor);
	if (!ASI)
	{
		return;
	}

	UGeoAbilitySystemComponent* TargetASC = Cast<UGeoAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
	UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!TargetASC || !SourceASC)
	{
		return;
	}

	UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(EffectDataArray, SourceASC, TargetASC,
														FMath::Max(1, FMath::RoundToInt32(PowerScale)), 0);

	OnRecalled();
}
