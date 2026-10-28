// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoGemDrop.h"

#include "Characters/PlayableCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemMeshes.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoGemDrop::AGeoGemDrop()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	Pickup = CreateDefaultSubobject<USphereComponent>(TEXT("Pickup"));
	Pickup->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pickup->SetCollisionObjectType(ECC_WorldDynamic);
	Pickup->SetCollisionResponseToAllChannels(ECR_Ignore);
	Pickup->SetCollisionResponseToChannel(ECC_GeoCharacter, ECR_Overlap);
	SetRootComponent(Pickup);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Pickup);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpawnSound.RandomPitchMultiplierRange = DropPitchRange;
	PickupSound.RandomPitchMultiplierRange = DropPitchRange;
}

void AGeoGemDrop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoGemDrop, Spec);
	DOREPLIFETIME(AGeoGemDrop, LaunchVelocity);
	DOREPLIFETIME(AGeoGemDrop, LaunchTime);
}

void AGeoGemDrop::Init(FGeoGemDropSpec const& InSpec, FVector const& InLaunchVelocity)
{
	if (ensureMsgf(GeoLib::IsServer(this), TEXT("%s: Init is server-only"), *GetName()))
	{
		Spec = InSpec;
		LaunchVelocity = InLaunchVelocity;
		LaunchTime = GeoLib::GetServerTime(this);
	}
}

void AGeoGemDrop::BeginPlay()
{
	Super::BeginPlay();

	Pickup->SetSphereRadius(PickupRadius);
	float const Elapsed = FMath::Max(0.f, GeoLib::GetServerTime(this) - LaunchTime);
	LocalLaunchTime = GetWorld()->GetTimeSeconds() - Elapsed;
	LaunchLocation = GetActorLocation();
	float const DropHeight = LaunchLocation.Z - RestHeight;
	ImpactSpeed = FMath::Sqrt(FMath::Max(0.f, FMath::Square(LaunchVelocity.Z) + 2.f * Gravity * DropHeight));
	FlightTime = (LaunchVelocity.Z + ImpactSpeed) / Gravity;
	BuildPath();

	FRandomStream Stream(Spec.Seed);
	TumbleAxis = Stream.VRand();
	TumbleRadians = FMath::DegreesToRadians(Stream.FRandRange(TumbleSpeed.Min, TumbleSpeed.Max));

	if (!GeoLib::IsDedicatedServer(this))
	{
		ApplyLook();
	}

	if (GeoLib::IsServer(this))
	{
		Pickup->OnComponentBeginOverlap.AddUniqueDynamic(this, &AGeoGemDrop::OnPickupOverlap);
		FTimerHandle PickupTimer;
		GetWorldTimerManager().SetTimer(PickupTimer, this, &AGeoGemDrop::EnablePickup, PickupDelay, false);
		if (TravelTime > 0.f)
		{
			FTimerHandle LandTimer;
			GetWorldTimerManager().SetTimer(LandTimer, this, &AGeoGemDrop::Land, TravelTime, false);
		}
		else
		{
			Land();
		}
	}

	if (!IsLanded())
	{
		PlayDropSound(SpawnSound, LaunchLocation);
	}
}

void AGeoGemDrop::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed && IsPickable())
	{
		PlayDropSound(PickupSound, GetActorLocation());
	}

	Super::EndPlay(EndPlayReason);
}

void AGeoGemDrop::PlayDropSound(FGeoSoundEntry const& Entry, FVector const& Location)
{
	UGeoSoundRowLibrary::PlaySoundEntryAtLocation(this, Entry, Location, this);
}

void AGeoGemDrop::ApplyLook()
{
	if (Spec.Kind == EGeoGemDropKind::Shard)
	{
		Mesh->SetStaticMesh(GeoGem::GetShardMesh(Spec.Variant).LoadSynchronous());
		Material = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		return;
	}

	UGeoGemCatalog const* const Catalog = UGeoGemCatalog::Get();
	FGeoGemInfo const* const Gem = Catalog ? Catalog->Find(Spec.GemId) : nullptr;
	if (ensureMsgf(Gem, TEXT("%s: no catalog gem %s to show"), *GetName(), *Spec.GemId.ToString()))
	{
		Mesh->SetStaticMesh(GeoGem::GetTierMesh(*Catalog->FindTier(Spec.GemId)).LoadSynchronous());
		Material = Mesh->CreateAndSetMaterialInstanceDynamic(0);
		Mesh->SetColorParameterValueOnMaterials(ColorParameter, Gem->Color.GetColor());
	}
}

bool AGeoGemDrop::IsLanded() const
{
	return GetWorld()->GetTimeSeconds() - LocalLaunchTime >= TravelTime;
}

bool AGeoGemDrop::IsPickable() const
{
	return GetWorld()->GetTimeSeconds() - LocalLaunchTime >= PickupDelay;
}

void AGeoGemDrop::Tick(float const DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float const Elapsed = GetWorld()->GetTimeSeconds() - LocalLaunchTime;
	SetActorLocation(GetThrowLocation(FMath::Min(Elapsed, TravelTime)));

	float const Resting = Elapsed - FlightTime;
	FGeoGemBounce const Bounce = GetBounce(Resting);
	float const Bob = BobHeight * FMath::Sin(Resting * BobSpeed) * GetSettled(Resting);
	Mesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, Bounce.Height + Bob), GetPose(Elapsed));
	Mesh->SetRelativeScale3D(FVector(GetPopScale(Elapsed)));
	if (Material && Bounce.Flash > 0.001f)
	{
		Material->SetScalarParameterValue(FlashParameter, Bounce.Flash);
	}
}

FGeoGemBounce AGeoGemDrop::GetBounce(float const RestingSeconds) const
{
	FGeoGemBounce Bounce;
	float Speed = ImpactSpeed * BounceRestitution;
	float Left = RestingSeconds;
	for (int32 Index = 0; Index < BounceCount && Left >= 0.f; ++Index)
	{
		float const Duration = 2.f * Speed / Gravity;
		Bounce.Flash = FMath::Pow(BounceRestitution, Index) * FMath::Exp(-FlashDecay * Left);
		Bounce.Height = Left < Duration ? Speed * Left - 0.5f * Gravity * FMath::Square(Left) : 0.f;
		Left -= Duration;
		Speed *= BounceRestitution;
	}

	return Bounce;
}

float AGeoGemDrop::GetSettled(float const RestingSeconds) const
{
	float const BounceSeconds = 2.f * ImpactSpeed * BounceRestitution / Gravity
		* (1.f - FMath::Pow(BounceRestitution, BounceCount)) / (1.f - BounceRestitution);
	float Settled = 0.f;
	if (RestingSeconds >= 0.f)
	{
		Settled = BounceSeconds > 0.f ? FMath::SmoothStep(0.f, 1.f, RestingSeconds / BounceSeconds) : 1.f;
	}

	return Settled;
}

FQuat AGeoGemDrop::GetPose(float const ElapsedSeconds) const
{
	return FQuat(TumbleAxis, TumbleRadians / TumbleFriction * (1.f - FMath::Exp(-TumbleFriction * ElapsedSeconds)));
}

float AGeoGemDrop::GetPopScale(float const ElapsedSeconds) const
{
	constexpr float Overshoot = 1.70158f;
	float const Remaining = FMath::Clamp(ElapsedSeconds / PopTime, 0.f, 1.f) - 1.f;
	return 1.f + (Overshoot + 1.f) * FMath::Cube(Remaining) + Overshoot * FMath::Square(Remaining);
}

void AGeoGemDrop::BuildPath()
{
	FCollisionObjectQueryParams const Objects(ECC_WorldStatic);
	FCollisionShape const Sphere = FCollisionShape::MakeSphere(CollisionRadius);
	FCollisionQueryParams const QueryParams(SCENE_QUERY_STAT(GeoGemDrop), false, this);

	FVector2D Velocity(LaunchVelocity);
	FVector2D Location(LaunchLocation);
	Path = {Location};
	while (Path.Num() < MaxPathPoints && ((Path.Num() - 1) * PathStep < FlightTime || Velocity.Size() > RestSpeed))
	{
		float const GroundSeconds = FMath::Clamp(Path.Num() * PathStep - FlightTime, 0.f, PathStep);
		Velocity *= FMath::Exp(-GroundFriction * GroundSeconds);
		FVector2D const Next = Location + Velocity * PathStep;

		FHitResult Hit;
		if (GetWorld()->SweepSingleByObjectType(Hit, FVector(Location, RestHeight), FVector(Next, RestHeight),
												FQuat::Identity, Objects, Sphere, QueryParams)
			&& !Hit.bStartPenetrating)
		{
			FVector2D const Normal = FVector2D(Hit.ImpactNormal).GetSafeNormal();
			Velocity = (Velocity - 2.f * FVector2D::DotProduct(Velocity, Normal) * Normal) * WallRestitution;
			Location = FVector2D(Hit.Location);
		}
		else
		{
			Location = Next;
		}

		Path.Add(Location);
	}

	TravelTime = FMath::Max(FlightTime, (Path.Num() - 1) * PathStep);
}

FVector AGeoGemDrop::GetThrowLocation(float const TravelSeconds) const
{
	float const PathIndex = FMath::Clamp(TravelSeconds / PathStep, 0.f, Path.Num() - 1.f);
	int32 const From = FMath::FloorToInt(PathIndex);
	FVector2D const Horizontal = FMath::Lerp(Path[From], Path[FMath::Min(From + 1, Path.Num() - 1)], PathIndex - From);

	float const AirSeconds = FMath::Min(TravelSeconds, FlightTime);
	float const Height = LaunchLocation.Z + LaunchVelocity.Z * AirSeconds - 0.5f * Gravity * FMath::Square(AirSeconds);
	return FVector(Horizontal, Height);
}

FVector AGeoGemDrop::GetRestLocation() const
{
	return GetThrowLocation(TravelTime);
}

void AGeoGemDrop::EnablePickup()
{
	Pickup->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AGeoGemDrop::Land()
{
	SetActorLocation(GetRestLocation());
	if (GeoLib::IsDedicatedServer(this))
	{
		SetActorTickEnabled(false);
	}
}

void AGeoGemDrop::OnPickupOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
								  UPrimitiveComponent* /*OtherComponent*/, int32 /*OtherBodyIndex*/,
								  bool /*bFromSweep*/, FHitResult const& /*SweepResult*/)
{
	if (Cast<APlayableCharacter>(OtherActor))
	{
		Destroy();
	}
}
