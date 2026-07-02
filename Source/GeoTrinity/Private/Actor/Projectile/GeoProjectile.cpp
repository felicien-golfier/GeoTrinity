// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoProjectile.h"

#include "AbilitySystem/Data/EffectData.h" //Necessary for array transfer.
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "Settings/GameDataSettings.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

static TAutoConsoleVariable CVarDrawServerProjectiles(TEXT("Geo.DrawServerProjectiles"), false,
													  TEXT("Draw debug spheres for projectiles on the server"));

static TAutoConsoleVariable
	CVarReplaceLocalProjectiles(TEXT("Geo.ReplaceLocalProjectiles"), false,
								TEXT("When true, server projectile Does replicate to owner and override local one"));

// ---------------------------------------------------------------------------------------------------------------------
AGeoProjectile::AGeoProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bAllowTickBeforeBeginPlay = false;
	bReplicates = true;

	SetCanBeDamaged(false);

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);

	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionProfileName(TEXT("GeoProjectile"));
	Sphere->SetCollisionObjectType(ECC_GeoProjectile);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	LoopingSoundComponent = CreateDefaultSubobject<UAudioComponent>("LoopingSoundComponent");
	LoopingSoundComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoProjectile, PredictionKeyId);
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoProjectile::IsNetRelevantFor(AActor const* RealViewer, AActor const* ViewTarget,
									  FVector const& SrcLocation) const
{
	// Decides if yes or not it needs to replicate on this client.
	// Here we don't want to replicate on the projectile owner.
	if (!CVarReplaceLocalProjectiles.GetValueOnGameThread() && PredictionKeyId != 0 && GetNetConnection()
		&& GetNetConnection() == RealViewer->GetNetConnection())
	{
		return false;
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!Implements<UGeoPoolableInterface>())
	{
		if (GeoLib::IsServer(GetWorld()))
		{
			// Blueprint construction (run by FinishSpawningActor) resets velocity to local (1,0,0); re-apply after.
			InitProjectileMovementComponent(); // TODO : find out how to remove this.
		}
		else
		{
			// Replicated server projectile arriving on client: InitProjectileLife was never called here.
			InitProjectileLife();
		}
	}

	// When a real (server-replicated) projectile arrives on the owning client,
	// find and destroy the matching predicted fake spawned earlier by the client.
	if (!CVarReplaceLocalProjectiles.GetValueOnGameThread() || PredictionKeyId == 0 || GeoLib::IsServer(GetWorld())
		|| Implements<UGeoPoolableInterface>())
	{
		return;
	}

	for (TActorIterator<AGeoProjectile> It(GetWorld(), GetClass()); It; ++It)
	{
		AGeoProjectile* Other = *It;
		if (Other != this && Other->PredictionKeyId == PredictionKeyId && Other->GetIsReplicated()
			&& Other->GetOwner() == GetOwner())
		{
			Other->Destroy();
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::LifeSpanExpired()
{
	if (LifeSpanInSec != 0 && !bIsEnding)
	{
		bIsEnding = true;
		EndProjectileLife();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsEnding)
	{
		return;
	}

	float const ElapsedDistanceSqr = FVector::DistSquared(GetActorLocation(), InitialPosition);
	if (ElapsedDistanceSqr >= DistanceSpanSqr)
	{
		bIsEnding = true;
		EndProjectileLife();
	}

	UE_VLOG_SPHERE(this, LogGeoTrinity, Verbose, GetActorLocation(), GetSimpleCollisionRadius(),
				   GeoLib::GetColorForObject(GetOuter()), TEXT("Projectile tick of %s"), *GetName());

	if (CVarDrawServerProjectiles.GetValueOnGameThread() && GeoLib::IsServer(GetWorld()))
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), GetSimpleCollisionRadius(), 8,
						GeoLib::GetColorForObject(GetOuter()), false, 0.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
									 UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex,
									 bool bFromSweep, FHitResult const& SweepResult)
{
	if (IsValidOverlap(OtherActor))
	{
		HandleValidOverlap(OtherActor);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoProjectile::IsValidOverlap(AActor* OtherActor)
{
	// When we are on a fully replicated projectile, Payload is not replicated, but Owner and Instigator are.
	AActor* const CurrentOwner = IsValid(Payload.Owner) ? Payload.Owner : GetOwner();
	AActor* const CurrentInstigator = IsValid(Payload.Instigator) ? Payload.Instigator : GetInstigator();

	if (!IsValid(CurrentOwner) || !IsValid(CurrentInstigator) || !IsValid(OtherActor))
	{
		return false;
	}

	if (!IsValid(GeoASLib::GetGeoAscFromActor(CurrentOwner))
		|| !IsValid(GeoASLib::GetGeoAscFromActor(CurrentInstigator))
		|| !IsValid(GeoASLib::GetGeoAscFromActor(OtherActor)))
	{
		return false;
	}

	if (bIsEnding)
	{
		return false;
	}

	if (!OtherActor->CanBeDamaged())
	{
		return false;
	}

	if (OtherActor == CurrentInstigator
		&& (!bCanOverlapInstigator || LifeSpanInSec - GetLifeSpan() < LifeTimeThresholdBeforeOverlapSelf))
	{
		return false;
	}

	return GeoASLib::IsTeamAttitudeAligned(CurrentOwner, OtherActor, OverlapAttitude);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::HandleValidOverlap(AActor* OtherActor)
{
	bIsEnding = true;

	if (HasAuthority())
	{
		GeoASLib::ApplyEffectFromEffectData(EffectDataArray, GeoASLib::GetGeoAscFromActor(Payload.Owner),
											GeoASLib::GetGeoAscFromActor(OtherActor), Payload.AbilityLevel,
											Payload.Seed, Payload.AbilityTag);
	}

	OnProjectileHit(OtherActor);
	EndProjectileLife();
}

float AGeoProjectile::GetPitch_Implementation(EProjectileSoundType SoundType) const
{
	FProjectilePitchEntry const* Entry = PitchMap.Find(SoundType);
	if (!Entry)
	{
		return 1.f;
	}

	float Pitch = 1.f;

	if (IsValid(Entry->Curve) && Entry->Attribute.IsValid())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
		if (IsValid(ASC))
		{
			bool bFound = false;
			float const AttributeValue = ASC->GetGameplayAttributeValue(Entry->Attribute, bFound);
			if (bFound)
			{
				Pitch = Entry->Curve->GetFloatValue(AttributeValue);
			}
		}
	}

	Pitch *= FMath::RandRange(Entry->RandomPitchMultiplierRange.X, Entry->RandomPitchMultiplierRange.Y);

	return Pitch;
}

void AGeoProjectile::OnProjectileHit_Implementation(AActor* HitActor)
{
	if (UGeoGameFeelComponent* GameFeel = HitActor->FindComponentByClass<UGeoGameFeelComponent>())
	{
		GameFeel->FlashOnHit();
	}
}

void AGeoProjectile::OnSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
								 FVector NormalImpulse, FHitResult const& Hit)

{
	if (bIsEnding || ProjectileMovement->bShouldBounce)
	{
		return;
	}

	bIsEnding = true;
	EndProjectileLife();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlayImpactFx() const
{
	if (!IsValid(this))
	{
		return;
	}

	FVector const actorLocation = GetActorLocation();
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, actorLocation, FRotator::ZeroRotator, 0.2f,
											  GetPitch(EProjectileSoundType::Impact));
	}
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, actorLocation);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::EndProjectileLife()
{
	PlayImpactFx();

	OnProjectileEndLifeDelegate.Broadcast(this);
	Destroy();
}

void AGeoProjectile::InitProjectileMovementComponent()
{
	// Clear any previous movement state
	ProjectileMovement->SetUpdatedComponent(GetRootComponent());

	if (ProjectileMovement->InitialSpeed > 0.f)
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		if (ProjectileMovement->bRotationFollowsVelocity)
		{
			SetActorRotation(ProjectileMovement->Velocity.Rotation());
		}
		ProjectileMovement->UpdateComponentVelocity();
	}
	else
	{
		ProjectileMovement->StopMovementImmediately();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::AdvanceProjectile(float const TimeDelta)
{
	if (TimeDelta <= 0.f || bIsEnding)
	{
		return;
	}

	FVector const CurrentLocation = GetActorLocation();
	FVector const Velocity = ProjectileMovement->Velocity;
	FVector const AdvancedPosition = CurrentLocation + Velocity * TimeDelta;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Payload.Instigator);

	FCollisionObjectQueryParams ObjectQueryParams = FCollisionObjectQueryParams(ECC_GeoCharacter); // GeoCharacter ECC
	ObjectQueryParams.AddObjectTypesToQuery(ECC_GeoProjectile);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	TArray<FHitResult> HitResults;
	bool const bHit = GetWorld()->LineTraceMultiByObjectType(HitResults, CurrentLocation, AdvancedPosition,
															 ObjectQueryParams, QueryParams);

	if (bHit)
	{
		for (FHitResult const& Hit : HitResults)
		{
			if (AActor* HitActor = Hit.GetActor(); HitActor && IsValidOverlap(HitActor))
			{
				SetActorLocation(Hit.ImpactPoint);
				if (Hit.bBlockingHit)
				{
					OnSphereHit(nullptr, HitActor, nullptr, FVector::ZeroVector, Hit);
				}
				else
				{
					OnSphereOverlap(nullptr, HitActor, nullptr, 0, false, Hit);
				}
				return;
			}
		}
	}

	SetActorLocation(AdvancedPosition);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OverrideDistanceSpan(float const Distance)
{
	bUseGeneralSpellDistance = false;
	DistanceSpan = Distance;
	DistanceSpanSqr = FMath::Square(DistanceSpan);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::InitProjectileLife()
{
	if (!GeoLib::IsDedicatedServer(this))
	{
		LoopingSoundComponent->SetSound(LoopingSound);
		LoopingSoundComponent->SetPitchMultiplier(GetPitch(EProjectileSoundType::Looping));
		LoopingSoundComponent->Play();
		UGameplayStatics::PlaySoundAtLocation(this, StartSound, GetActorLocation(), FRotator::ZeroRotator, 0.2f,
											  GetPitch(EProjectileSoundType::Start));
	}

	SetLifeSpan(LifeSpanInSec);
	Sphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnSphereHit);

	InitialPosition = GetActorLocation();
	DistanceSpanSqr =
		FMath::Square(bUseGeneralSpellDistance ? GetDefault<UGameDataSettings>()->GeneralSpellDistance : DistanceSpan);
	InitProjectileMovementComponent();

	bIsEnding = false;
}
