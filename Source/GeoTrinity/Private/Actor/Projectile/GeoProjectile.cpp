// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Projectile/GeoProjectile.h"

#include "AbilitySystem/Data/EffectData.h" //Necessary for array transfer.
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Characters/GeoCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Settings/GameDataSettings.h"
#include "System/GeoPoolableInterface.h"
#include "Tool/GeoNiagaraParams.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "UObject/ConstructorHelpers.h"

static TAutoConsoleVariable CVarDrawServerProjectiles(TEXT("Geo.DrawServerProjectiles"), false,
													  TEXT("Draw debug spheres for projectiles on the server"));

static TAutoConsoleVariable
	CVarReplaceLocalProjectiles(TEXT("Geo.ReplaceLocalProjectiles"), false,
								TEXT("When true, server projectile Does replicate to owner and override local one"));

// Rate the visual slides back onto the actor at (SetVisualLaunchLocation): ~1% of the offset is left after 0.2s, short
// enough that the bullet is on its true path well before anything can be read off its position.
static constexpr float VisualCatchUpSpeed = 25.f;

namespace
{
	template <typename T>
	T ResolveOverrideParam(EOverrideParam Mode, T const& OverrideValue, T const& SettingsValue,
						   T const& BlueprintDefault)
	{
		switch (Mode)
		{
		case EOverrideParam::OverrideValue:
			return OverrideValue;
		case EOverrideParam::UseGameDataSettings:
			return SettingsValue;
		case EOverrideParam::KeepBlueprintDefaultValue:
			return BlueprintDefault;
		}
		return BlueprintDefault;
	}

	/** Overload for params with no UGameDataSettings counterpart: UseGameDataSettings resolves to the Blueprint
	 * default, exactly like KeepBlueprintDefaultValue. */
	template <typename T>
	T ResolveOverrideParam(EOverrideParam Mode, T const& OverrideValue, T const& BlueprintDefault)
	{
		return ResolveOverrideParam(Mode, OverrideValue, BlueprintDefault, BlueprintDefault);
	}
} // namespace

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

	BulletVFX = CreateDefaultSubobject<UNiagaraComponent>("BulletVFX");
	BulletVFX->SetupAttachment(Sphere);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BulletSystem(
		TEXT("/Game/VFX/Assets/NS_GeoTrinity_Projectile01.NS_GeoTrinity_Projectile01"));
	if (BulletSystem.Succeeded())
	{
		BulletVFX->SetAsset(BulletSystem.Object);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PostInitProperties()
{
	Super::PostInitProperties();
	ResolvedParams = DefaultParams;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoProjectile, PredictionKeyId);
	DOREPLIFETIME(AGeoProjectile, ReplicatedSpeed);
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

	// Simulated proxies never run ApplyProjectileParams (that happens where the projectile is spawned), so give them
	// the per-Blueprint params here. Authoritative and predicted instances are locally ROLE_Authority and already
	// applied it.
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		ApplyParams(DefaultParams);
	}

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
	if (LifeSpanInSec != 0)
	{
		EndProjectileLife();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Destroyed()
{
	if (!HasAuthority() && !bIsEnding)
	{
		PlayImpactFx();
	}
	Super::Destroyed();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsEnding)
	{
		return;
	}

	// A visual placed off the actor by SetVisualLaunchLocation slides home, so what the player sees converges onto the
	// trajectory that actually counts. Exactly zero on every projectile that never asked for it.
	FVector const VisualOffset = BulletVFX->GetRelativeLocation();
	if (!VisualOffset.IsNearlyZero())
	{
		BulletVFX->SetRelativeLocation(
			FMath::VInterpTo(VisualOffset, FVector::ZeroVector, DeltaSeconds, VisualCatchUpSpeed));
	}

	float const ElapsedDistanceSqr = FVector::DistSquared(GetActorLocation(), InitialPosition);
	if (ElapsedDistanceSqr >= DistanceSpanSqr)
	{
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
	UGeoAbilitySystemComponent* OwnerASC = nullptr;
	UGeoAbilitySystemComponent* TargetASC = nullptr;
	if (IsValidOverlap(OtherActor, OwnerASC, TargetASC))
	{
		HandleValidOverlap(OtherActor, OwnerASC, TargetASC);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
bool AGeoProjectile::IsValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent*& OutOwnerASC,
									UGeoAbilitySystemComponent*& OutTargetASC)
{
	AActor* CurrentOwner = Payload.Owner;
	if (!IsValid(CurrentOwner))
	{
		// Expected on a simulated proxy: Payload does not replicate, so a server-spawned projectile only carries the
		// replicated Owner there. Anywhere else it means the spawn never filled the payload.
		CurrentOwner = GetOwner();
		UE_LOG(LogGeoTrinity, Verbose, TEXT("%hs: %s has no Payload.Owner, falling back to the replicated Owner %s"),
			   __FUNCTION__, *GetName(), *GetNameSafe(CurrentOwner));
	}

	AActor* const CurrentInstigator = GetCurrentInstigator();

	if (bIsEnding || !IsValid(CurrentOwner) || !IsValid(CurrentInstigator) || !IsValid(OtherActor)
		|| !OtherActor->CanBeDamaged())
	{
		return false;
	}

	OutOwnerASC = GeoASLib::GetGeoAscFromActor(CurrentOwner);
	OutTargetASC = GeoASLib::GetGeoAscFromActor(OtherActor);
	if (!IsValid(OutOwnerASC) || !IsValid(OutTargetASC)
		|| !IsValid(GeoASLib::GetGeoAscFromActor(CurrentInstigator)))
	{
		return false;
	}

	if (OtherActor == CurrentInstigator
		&& (!ResolvedParams.bCanOverlapInstigator
			|| LifeSpanInSec - GetLifeSpan() < ResolvedParams.LifeTimeThresholdBeforeOverlapSelf))
	{
		return false;
	}

	return GeoASLib::IsTeamAttitudeAligned(CurrentOwner, OtherActor, ResolvedParams.OverlapAttitude);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::HandleValidOverlap(AActor* OtherActor, UGeoAbilitySystemComponent* OwnerASC,
										UGeoAbilitySystemComponent* TargetASC)
{
	EndSoundType = EProjectileSoundType::ValidOverlapEnd;

	if (GeoLib::IsServer(this))
	{
		GeoASLib::ApplyEffectFromEffectData(EffectDataArray, OwnerASC, TargetASC, Payload.AbilityLevel, Payload.Seed,
											Payload.AbilityTag);
	}

	OnProjectileHit(OtherActor);
	EndProjectileLife();
}

float AGeoProjectile::GetPitch_Implementation(EProjectileSoundType SoundType) const
{
	FGeoSoundEntry const* Entry = ResolvedParams.SoundMap.Find(SoundType);
	return Entry ? GetPitch(*Entry) : 1.f;
}

// ---------------------------------------------------------------------------------------------------------------------
float AGeoProjectile::GetPitch(FGeoSoundEntry const& Entry) const
{
	return UGeoSoundRowLibrary::GetPitch(Entry, GetCurrentInstigator());
}

// ---------------------------------------------------------------------------------------------------------------------
AActor* AGeoProjectile::GetCurrentInstigator() const
{
	// When we are on a fully replicated projectile, Payload is not replicated, but Instigator is.
	return IsValid(Payload.Instigator) ? Payload.Instigator : GetInstigator();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlaySoundOneShot(EProjectileSoundType const SoundType) const
{
	if (FGeoSoundEntry const* Entry = ResolvedParams.SoundMap.Find(SoundType))
	{
		PlaySoundOneShot(*Entry);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlaySoundOneShot(FGeoSoundEntry const& Entry) const
{
	if (UGeoSoundRowLibrary::ShouldPlay(this, Entry, GetCurrentInstigator()))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Entry.Sound, GetActorLocation(), FRotator::ZeroRotator,
											  UGeoSoundRowLibrary::GetVolume(Entry, GetCurrentInstigator()),
											  GetPitch(Entry), Entry.StartTime);
	}
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
	if (ProjectileMovement->bShouldBounce)
	{
		return;
	}

	EndProjectileLife();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PlayImpactFx() const
{
	if (GeoLib::IsDedicatedServer(this))
	{
		return;
	}

	PlaySoundOneShot(EndSoundType);
	if (EndSoundType == EProjectileSoundType::ValidOverlapEnd)
	{
		PlaySoundOneShot(EProjectileSoundType::NoOverlapEnd);
	}

	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::EndProjectileLife()
{
	if (bIsEnding)
	{
		return;
	}
	bIsEnding = true;
	UnbindFromInstigatorRevive();

	PlayImpactFx();

	OnProjectileEndLifeDelegate.Broadcast(this);

	// A simulated proxy must never Destroy() itself: the server still replicates the actor, and any later property
	// bunch (e.g. a bounce snapshot) re-creates it client-side as a fresh, wrongly-moving ghost. Go dark instead and
	// let the server's replicated destruction remove the actor. Predicted fakes are client-spawned and therefore
	// locally ROLE_Authority, so they still destroy themselves here.
	if (GetLocalRole() < ROLE_Authority)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		ProjectileMovement->StopMovementImmediately();
		LoopingSoundComponent->Stop();
		return;
	}

	Destroy();
}

void AGeoProjectile::InitProjectileMovementComponent()
{
	if (ReplicatedSpeed > 0.f)
	{
		ProjectileMovement->InitialSpeed = ProjectileMovement->MaxSpeed = ReplicatedSpeed;
	}

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

	TArray<FHitResult> HitResults;
	bool const bHit = GetWorld()->LineTraceMultiByChannel(HitResults, CurrentLocation, AdvancedPosition,
														  ECC_GeoProjectile, QueryParams);

	if (bHit)
	{
		for (FHitResult const& Hit : HitResults)
		{
			AActor* const HitActor = Hit.GetActor();
			if (!HitActor)
			{
				continue;
			}

			UGeoAbilitySystemComponent* OwnerASC = nullptr;
			UGeoAbilitySystemComponent* TargetASC = nullptr;
			if (IsValidOverlap(HitActor, OwnerASC, TargetASC))
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

			if (Hit.bBlockingHit)
			{
				SetActorLocation(Hit.ImpactPoint);
				OnSphereHit(nullptr, HitActor, nullptr, FVector::ZeroVector, Hit);
				return;
			}
		}
	}

	SetActorLocation(AdvancedPosition);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OverrideDistanceSpan(float const Distance)
{
	ResolvedParams.DistanceSpan = Distance;
	DistanceSpanSqr = FMath::Square(Distance);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OverrideSpeed(float const Speed)
{
	ResolvedParams.ProjectileSpeed = Speed;
	ReplicatedSpeed = Speed;
	ProjectileMovement->InitialSpeed = ProjectileMovement->MaxSpeed = Speed;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::SetVisualLaunchLocation(FVector const& WorldLocation)
{
	BulletVFX->SetWorldLocation(WorldLocation);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::ApplyParams(FProjectileParamsBase const& Params)
{
	ResolvedParams = Params;

	Sphere->SetSphereRadius(Params.Radius);
	BulletVFX->SetVariableFloat(GeoNiagaraParams::BulletRadius, Params.Radius);
	BulletVFX->SetVariableLinearColor(GeoNiagaraParams::BulletHeadColor, Params.HeadColor.GetColor(1.f));
	BulletVFX->SetVariableLinearColor(GeoNiagaraParams::BulletTrailColor, Params.TrailColor.GetColor(1.f));
	BulletVFX->SetVariableFloat(GeoNiagaraParams::TrailLifetimeScale, Params.TrailLifetimeScale);
}

#if WITH_EDITOR
// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PreviewParams()
{
	ApplyParams(DefaultParams);
	BulletVFX->ReinitializeSystem();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() != GET_MEMBER_NAME_CHECKED(AGeoProjectile, DefaultParams))
	{
		return;
	}

	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		PreviewParams();
		return;
	}

	// The property editor already propagated the new value to every instance before calling this, so each one can just
	// preview its own DefaultParams.
	TArray<UObject*> Instances;
	GetArchetypeInstances(Instances);
	for (UObject* Instance : Instances)
	{
		if (!Instance->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			CastChecked<AGeoProjectile>(Instance)->PreviewParams();
		}
	}
}
#endif

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::ApplyProjectileParams(FExternalProjectileParams const& Params)
{
	UGameDataSettings const* Settings = GetDefault<UGameDataSettings>();

	// Only distance, speed and radius have a settings value; every other param resolves UseGameDataSettings to
	// DefaultParams, same as KeepBlueprintDefaultValue, so only an explicit OverrideValue changes it. The
	// KeepBlueprintDefaultValue speed is the movement component's own InitialSpeed, not a DefaultParams value.
	FProjectileParamsBase Resolved;
	Resolved.DistanceSpan = ResolveOverrideParam(Params.OverrideDistanceSpan, Params.DistanceSpan,
												 Settings->GeneralSpellDistance, DefaultParams.DistanceSpan);
	Resolved.ProjectileSpeed = ResolveOverrideParam(Params.OverrideSpeed, Params.ProjectileSpeed,
													Settings->GeneralSpellSpeed, ProjectileMovement->InitialSpeed);
	Resolved.Radius = ResolveOverrideParam(Params.OverrideRadius, Params.Radius, Settings->GeneralProjectileRadius,
										   DefaultParams.Radius);
	Resolved.HeadColor = ResolveOverrideParam(Params.OverrideHeadColor, Params.HeadColor, DefaultParams.HeadColor);
	Resolved.TrailColor = ResolveOverrideParam(Params.OverrideTrailColor, Params.TrailColor, DefaultParams.TrailColor);
	Resolved.TrailLifetimeScale = ResolveOverrideParam(Params.OverrideTrailLifetimeScale, Params.TrailLifetimeScale,
													   DefaultParams.TrailLifetimeScale);
	Resolved.SoundMap = ResolveOverrideParam(Params.OverrideSoundMap, Params.SoundMap, DefaultParams.SoundMap);
	Resolved.OverlapAttitude =
		ResolveOverrideParam(Params.OverrideOverlapAttitude, Params.OverlapAttitude, DefaultParams.OverlapAttitude);
	Resolved.bCanOverlapInstigator = ResolveOverrideParam(
		Params.OverrideCanOverlapInstigator, Params.bCanOverlapInstigator, DefaultParams.bCanOverlapInstigator);
	// The self-overlap threshold intentionally shares bCanOverlapInstigator's override mode: the two describe one
	// decision, so overriding self-overlap without its delay would be meaningless.
	Resolved.LifeTimeThresholdBeforeOverlapSelf =
		ResolveOverrideParam(Params.OverrideCanOverlapInstigator, Params.LifeTimeThresholdBeforeOverlapSelf,
							 DefaultParams.LifeTimeThresholdBeforeOverlapSelf);

	ApplyParams(Resolved);

	if (Params.OverrideSpeed != EOverrideParam::KeepBlueprintDefaultValue)
	{
		OverrideSpeed(Resolved.ProjectileSpeed);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::InitProjectileLife()
{
	if (!GeoLib::IsDedicatedServer(this))
	{
		if (FGeoSoundEntry const* LoopingEntry = ResolvedParams.SoundMap.Find(EProjectileSoundType::Looping))
		{
			UGeoSoundRowLibrary::ConfigureAudioComponent(LoopingSoundComponent, *LoopingEntry, GetCurrentInstigator(),
														 GetPitch(EProjectileSoundType::Looping));
		}
		PlaySoundOneShot(EProjectileSoundType::Start);
	}

	SetLifeSpan(LifeSpanInSec);
	Sphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnSphereOverlap);
	Sphere->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnSphereHit);

	InitialPosition = GetActorLocation();
	DistanceSpanSqr = FMath::Square(ResolvedParams.DistanceSpan);
	InitProjectileMovementComponent();
	// A pooled instance can come back holding the previous shot's launch offset; the spawner re-applies its own after
	// this runs.
	BulletVFX->SetRelativeLocation(FVector::ZeroVector);

	bIsEnding = false;
	EndSoundType = EProjectileSoundType::NoOverlapEnd;

	ReviveBoundInstigator = Cast<AGeoCharacter>(GetCurrentInstigator());
	if (ReviveBoundInstigator.IsValid())
	{
		InstigatorRevivedHandle =
			ReviveBoundInstigator->OnRevived.AddUObject(this, &AGeoProjectile::OnInstigatorRevived);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::UnbindFromInstigatorRevive()
{
	if (ReviveBoundInstigator.IsValid())
	{
		ReviveBoundInstigator->OnRevived.Remove(InstigatorRevivedHandle);
	}
	ReviveBoundInstigator = nullptr;
	InstigatorRevivedHandle.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoProjectile::OnInstigatorRevived()
{
	EndProjectileLife();
}
