// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableBase.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoCombattantWidgetHost.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

// M_PulseCircle's DurationSpent parameter reads this slot. Custom primitive data is addressed by index and never by
// name, so this constant and the material's PrimitiveDataIndex are the whole contract between them.
int32 constexpr DurationSpentPrimitiveDataIndex = 0;

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoShape"));
	PrimaryActorTick.bCanEverTick = true;

	WidgetAnchorComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WidgetAnchorComponent"));
	WidgetAnchorComponent->SetupAttachment(GetRootComponent());
	WidgetAnchorComponent->SetUsingAbsoluteRotation(true);

	OutlineColor = EGeoColor::DeployableNotBlocking;
	// Concrete class comes from settings (a soft path) so gameplay never names the UI-module UGeoCombattantWidgetComp.
	// Optional subobject: the dedicated-server target doesn't ship the UI class, so it resolves to null and is skipped.
	if (UClass* const WidgetComponentClass =
			GetDefault<UGameDataSettings>()->CombattantWidgetComponentClass.LoadSynchronous())
	{
		CombattantWidgetComponent = Cast<UWidgetComponent>(ObjectInitializer.CreateDefaultSubobject(
			this, TEXT("CombattantWidgetComponent"), UWidgetComponent::StaticClass(), WidgetComponentClass,
			/*bIsRequired=*/false, /*bIsTransient=*/false));
		CombattantWidgetComponent->SetupAttachment(WidgetAnchorComponent);
		CombattantWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	}
}


void AGeoDeployableBase::InitInteractable(FInteractableActorData* Data)
{
	Super::InitInteractable(Data);

	bBlinking = false;
	bActive = true;
	bRecalled = false;
	if (bPushActorsOnSpawn)
	{
		PushAway();
	}
}

void AGeoDeployableBase::PushAway()
{
	AGeoArena const* FightingArena = AGeoArena::GetFightingArena(this);

	FVector2D const Location2D(GetActorLocation());
	float const Radius = CapsuleComponent->GetScaledCapsuleRadius();

	for (AActor* Actor :
		 GeoASLib::GetInteractableActors(this, FGenericTeamId::NoTeam, TeamAttitudeMask::All, true, Location2D, Radius))
	{
		ACharacter* Character = Cast<ACharacter>(Actor);
		UCharacterMovementComponent* Movement = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
		if (!IsValid(Movement))
		{
			continue;
		}

		float const PushDistance = Radius + Actor->GetSimpleCollisionRadius();
		ApplyPushRootMotion(Movement, Actor->GetActorLocation(), ComputePushTarget(Actor, PushDistance, FightingArena));
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
FVector AGeoDeployableBase::ComputePushTarget(AActor* Target, float const PushDistance,
											  AGeoArena const* FightingArena) const
{
	FVector PushDirection = Target->GetActorLocation() - GetActorLocation();
	PushDirection.Z = 0.f;
	if (PushDirection.IsNearlyZero())
	{
		PushDirection = FVector(1.f, 0.f, 0.f);
	}
	PushDirection.Normalize();

	FVector const PushTarget = Target->GetActorLocation() + PushDirection * PushDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Target);
	if (!GetWorld()->SweepTestByChannel(Target->GetActorLocation(), PushTarget, FQuat::Identity, ECC_Pawn,
										FCollisionShape::MakeSphere(Target->GetSimpleCollisionRadius()), QueryParams))
	{
		return PushTarget;
	}

	if (!FightingArena)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("%s: blocked push outside a fight — no arena centre to redirect toward"),
			   *GetName());
		return PushTarget;
	}

	// Pushing outward is blocked, so push toward the arena's centre instead — the one direction guaranteed to be
	// inside the fight.
	TArray<AActor*> const FightCenters =
		GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_FightCenter, FightingArena->ArenaTag);
	if (!ensureMsgf(!FightCenters.IsEmpty(), TEXT("%hs: no TargetPoint.FightCenter point in arena %s"), __FUNCTION__,
					*FightingArena->ArenaTag.ToString()))
	{
		return PushTarget;
	}

	FVector ToCenter = FightCenters[0]->GetActorLocation() - GetActorLocation();
	ToCenter.Z = 0.f;
	FVector RedirectedTarget = GetActorLocation() + ToCenter.GetSafeNormal() * PushDistance;
	RedirectedTarget.Z = Target->GetActorLocation().Z;
	return RedirectedTarget;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::ApplyPushRootMotion(UCharacterMovementComponent* Movement, FVector const& From,
											 FVector const& To)
{
	constexpr float PushDuration = 0.15f;

	TSharedPtr<FRootMotionSource_MoveToForce> PushRootMotion = MakeShared<FRootMotionSource_MoveToForce>();
	PushRootMotion->InstanceName = TEXT("PillarPush");
	PushRootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	PushRootMotion->StartLocation = From;
	PushRootMotion->TargetLocation = To;
	PushRootMotion->Duration = PushDuration;
	PushRootMotion->bRestrictSpeedToExpected = false;
	PushRootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	PushRootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	uint16 const SourceID = Movement->ApplyRootMotionSource(PushRootMotion);

	FTimerHandle RemoveHandle;
	GetWorldTimerManager().SetTimer(RemoveHandle,
									FTimerDelegate::CreateWeakLambda(Movement,
																	 [Movement, SourceID]()
																	 {
																		 Movement->RemoveRootMotionSourceByID(SourceID);
																		 Movement->SetMovementMode(MOVE_Falling);
																	 }),
									PushDuration, false);
}

void AGeoDeployableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGeoDeployableBase, bActive);
	DOREPLIFETIME(AGeoDeployableBase, bRecalled);
	DOREPLIFETIME(AGeoDeployableBase, bBlinking);
}

void AGeoDeployableBase::InitDrain()
{
	if (!GeoLib::IsServer(GetWorld()) || GetData()->Params.LifeDrainMaxDuration <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (!ensureMsgf(MaxHealth > 0.f, TEXT("%hs: MaxHealth is 0 on %s — DefaultAttributes may not be applied"),
					__FUNCTION__, *GetName()))
	{
		return;
	}

	DrainMagnitudePerSecond = MaxHealth / GetData()->Params.LifeDrainMaxDuration;

	// The drain never changes for this deployable — Tick only hands it the length of the tick it covers.
	DrainEffectData.Amount = DrainMagnitudePerSecond;
	DrainEffectData.bIsPerSecond = true;
	DrainEffectData.bSuppressGameplayCue = bSuppressDrainDamageVisuals;
	DrainEffectData.bDoNotRedirectSacrifice = !bCanSacrificeDrain;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float const DurationSpent = 1.f - GetDrainDurationRatio();
	for (UMeshComponent* const MeshComponent : GetVisualMeshComponents())
	{
		MeshComponent->SetCustomPrimitiveDataFloat(DurationSpentPrimitiveDataIndex, DurationSpent);
	}

	if (bUseRegularDrain && DrainMagnitudePerSecond > 0.f && bActive && !IsBlinking() && GeoLib::IsServer(GetWorld()))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		UGeoAbilitySystemLibrary::ApplySingleEffectData(DrainEffectData, ASC, ASC, GetData()->Level, GetData()->Seed,
														GetData()->AbilityTag);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	if (bPushActorsOnSpawn)
	{
		SetActorEnableCollision(false);
		GetWorldTimerManager().SetTimer(CollisionEnableTimerHandle, this, &ThisClass::EnableActorCollision,
										CollisionEnableDelay, false);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::EnableActorCollision()
{
	SetActorEnableCollision(true);
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::ApplyOutlineStencil() const
{
	if (!ensureMsgf(OutlineColor != EGeoColor::Override,
					TEXT("%s: OutlineColor is Override — the outline post-process resolves palette slots only"),
					*GetName()))
	{
		return;
	}

	// Stencil 0 means "no outline" to the post-process, so the slot index is shifted up by one.
	uint8 const StencilValue = static_cast<uint8>(OutlineColor) + 1;

	for (UMeshComponent* const MeshComponent : GetVisualMeshComponents())
	{
		MeshComponent->SetRenderCustomDepth(true);
		MeshComponent->SetCustomDepthStencilValue(StencilValue);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

TInlineComponentArray<UMeshComponent*> AGeoDeployableBase::GetVisualMeshComponents() const
{
	TInlineComponentArray<UMeshComponent*> MeshComponents(this);
	MeshComponents.RemoveAll(
		[](UMeshComponent const* MeshComponent)
		{
			return MeshComponent->IsA<UWidgetComponent>();
		});
	return MeshComponents;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::BeginPlay()
{
	Super::BeginPlay();

	ApplyOutlineStencil();

	if (CombattantWidgetComponent)
	{
		// The component's class is resolved at runtime, so its Details panel can't expose these — apply the per-BP
		// values here instead, where BP CDO overrides are loaded.
		UClass* const WidgetClass = HealthBarWidgetClassOverride.IsNull()
			? GetDefault<UGameDataSettings>()->DefaultDeployableHealthBarWidgetClass.LoadSynchronous()
			: HealthBarWidgetClassOverride.LoadSynchronous();
		if (WidgetClass)
		{
			CombattantWidgetComponent->SetWidgetClass(WidgetClass);
		}
		CombattantWidgetComponent->SetDrawAtDesiredSize(false);
		CombattantWidgetComponent->SetDrawSize(HealthBarDrawSize);
		CombattantWidgetComponent->SetRelativeLocation(HealthBarLocation);

		// Attributes are set synchronously in Super::BeginPlay, so bind the bar now that they exist (and after the
		// widget class is set so the user widget is created), or it reads MaxHealth as 0 and never updates (mirrors
		// AGeoCharacter::BeginPlay).
		CombattantWidgetComponent->InitWidget();
		if (IGeoCombattantWidgetHost* WidgetHost = Cast<IGeoCombattantWidgetHost>(CombattantWidgetComponent))
		{
			WidgetHost->BindToOwnerASC();
		}
	}

	if (GetInstigator())
	{
		if (UGeoDeployableManagerComponent* DeployableManager =
				GetInstigator()->GetComponentByClass<UGeoDeployableManagerComponent>())
		{
			DeployableManager->RegisterDeployable(this);
		}
	}

	InitDrain();

	if (!CanBeDamaged() && CombattantWidgetComponent)
	{
		CombattantWidgetComponent->SetHiddenInGame(true);
	}

	if (!GeoLib::IsDedicatedServer(this))
	{
		GeoASLib::ExecuteGeoCue(GetAbilitySystemComponent(), SpawnCue, GetSpawnCueParams(), true);
	}
	PlaySoundOneShot(EDeployableSoundType::Spawn);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Recall(float Value)
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	bRecalled = true;

	if (bExplodeAtRecall)
	{
		ExplodeEffect(Value);
	}
	PlayRecallCosmetics(Value);
	Expire();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::PlayRecallCosmetics(float const Value)
{
	// Local cues: run on every rendering machine incl. the listen-server host; skip only the dedicated server.
	bool const bRendersLocally = !GeoLib::IsDedicatedServer(this);

	if (bRendersLocally && RecallCue.IsValid())
	{
		GeoASLib::ExecuteGeoCue(GetAbilitySystemComponent(), RecallCue, GetRecallCueParams(), true);
	}
	PlaySoundOneShot(EDeployableSoundType::Recall);

	if (!bExplodeAtRecall)
	{
		return;
	}

	if (bRendersLocally && ExplodeCue.IsValid())
	{
		FGameplayCueParameters CueParams = GetGenericCueParams(ExplodeCue);
		// Value is server-side only (never replicated), so a client's copy of this cue always carries 0.
		CueParams.Normal.X = Value;
		GeoASLib::ExecuteGeoCue(GetAbilitySystemComponent(), ExplodeCue, CueParams, true);
	}
	PlaySoundOneShot(EDeployableSoundType::Explode);
}


// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::PlaySoundOneShot(EDeployableSoundType const SoundType) const
{
	if (FGeoSoundEntry const* Entry = SoundMap.Find(SoundType))
	{
		UGeoSoundRowLibrary::PlaySoundEntry2D(this, *Entry, GetData()->Instigator);
	}
}

void AGeoDeployableBase::ExplodeEffect(float const Value)
{
	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(GetData()->Owner);
	if (!ensureMsgf(SourceASC, TEXT("AGeoDeployableBase: no ASC on Owner")))
	{
		return;
	}

	TArray<AActor*> OverlappingActors =
		GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(GetData()->Owner), ExplodeAttitude, true,
										FVector2D(GetActorLocation()), GetData()->Params.Size, ExplodeOverlapMode);

	for (AActor* Actor : OverlappingActors)
	{
		UGeoAbilitySystemComponent* ActorASC = GeoASLib::GetGeoAscFromActor(Actor);
		if (IsValid(ActorASC))
		{
			GeoASLib::ApplyEffectFromEffectData(GetData()->EffectDataArray, SourceASC, ActorASC, GetData()->Level,
												GetData()->Seed, GetData()->AbilityTag);
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Expire(bool const bForce)
{
	bActive = false;
	bBlinking = false;
	GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BlinkVisibilityTimerHandle);
	SetActorHiddenInGame(true);
	OnDeployableExpiredEvent.Broadcast(this);
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
	if (!bForce)
	{
		if (!GeoLib::IsDedicatedServer(this))
		{
			GeoASLib::ExecuteGeoCue(GetAbilitySystemComponent(), ExpireCue, GetGenericCueParams(ExpireCue), true);
		}
		PlaySoundOneShot(EDeployableSoundType::Expire);
		if (!bRecalled)
		{
			PlaySoundOneShot(EDeployableSoundType::ExpireButNotRecalled);
		}
	}

	// A simulated proxy must never Destroy() itself: the server still replicates the actor, and a later property bunch
	// would re-create it client-side. Stay dark (hidden, tick off) and let the server's replicated destruction remove
	// the actor (mirrors AGeoProjectile::EndProjectileLife).
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	if (!bForce && TimeBeforeDestroyAtExpire > 0.f)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle,
										FTimerDelegate::CreateWeakLambda(this,
																		 [this]()
																		 {
																			 Destroy();
																		 }),
										TimeBeforeDestroyAtExpire, false);
	}
	else
	{
		Destroy();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float AGeoDeployableBase::GetDrainDurationRatio() const
{
	if (GetData()->Params.LifeDrainMaxDuration <= 0.f)
	{
		return 1.f;
	}

	UAbilitySystemComponent const* ASC = GetAbilitySystemComponent();
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(ASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()) / MaxHealth, 0.f, 1.f);
}

void AGeoDeployableBase::StartBlinking()
{
	bBlinking = true;
	if (!bDamageableDuringBlink)
	{
		SetActorEnableCollision(false);
		SetCanBeDamaged(false);
	}

	GetWorld()->GetTimerManager().SetTimer(BlinkTimerHandle, this, &ThisClass::TryRecallOrExpire,
										   GetData()->Params.BlinkDuration, false);

	// Local cue: run on every rendering machine incl. the listen-server host; skip only the dedicated server.
	if (BlinkingCue.IsValid() && !GeoLib::IsDedicatedServer(this))
	{
		GeoASLib::ExecuteGeoCue(GetAbilitySystemComponent(), BlinkingCue, GetBlinkCueParams(), true);
	}
	PlaySoundOneShot(EDeployableSoundType::Blinking);

	OnBlinkStart();
}
// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnHealthChanged_Implementation(float NewValue)
{
	if (NewValue <= 0.f && bActive && !IsBlinking())
	{
		if (GetData()->Params.BlinkDuration)
		{
			StartBlinking();
		}
		else
		{
			TryRecallOrExpire();
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkStart_Implementation()
{
	float constexpr BlinkRate = 0.2f;
	GetWorld()->GetTimerManager().SetTimer(BlinkVisibilityTimerHandle, this, &ThisClass::OnBlinkVisibilityTick,
										   BlinkRate, true);
}

void AGeoDeployableBase::OnRep_Active(bool bOldValue)
{
	if (bOldValue && !bActive)
	{
		if (bRecalled)
		{
			PlayRecallCosmetics(0.f);
		}
		Expire();
	}
}
void AGeoDeployableBase::OnRep_Blinking(bool bOldValue)
{
	if (!bOldValue && bBlinking && GetData()->Params.BlinkDuration > 0.f)
	{
		StartBlinking();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkVisibilityTick()
{
	SetActorHiddenInGame(!IsHidden());
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::TryRecallOrExpire()
{
	if (bActive)
	{
		if (bAutoRecallAtEndLife)
		{
			Recall();
		}
		else
		{
			Expire();
		}
	}
}

bool AGeoDeployableBase::IsBlinking() const
{
	return bBlinking;
}


// ---------------------------------------------------------------------------------------------------------------------

FGameplayCueParameters AGeoDeployableBase::GetSpawnCueParams() const
{
	FGameplayCueParameters CueParams = GetGenericCueParams(SpawnCue);
	CueParams.Normal.X = GetData()->Params.LifeDrainMaxDuration;
	return CueParams;
}

FGameplayCueParameters AGeoDeployableBase::GetBlinkCueParams() const
{
	FGameplayCueParameters CueParams = GetGenericCueParams(BlinkingCue);
	CueParams.Normal.X = GetData()->Params.BlinkDuration;
	return CueParams;
}

FGameplayCueParameters AGeoDeployableBase::GetGenericCueParams(FGeoCueParam const& Cue) const
{
	FVector Location = GetActorLocation();
	// TODO: find a better solution
	Location.Z = 1.f; // Ensure all Cues happens just above the floor

	FGameplayCueParameters CueParams = Cue.MakeCueParams(GetData()->Instigator, const_cast<AGeoDeployableBase*>(this),
														 Location, GetData()->Level, GetData()->AbilityTag);
	CueParams.RawMagnitude = GetData()->Params.Size;
	CueParams.NormalizedMagnitude = GetData()->Params.Value;
	return CueParams;
}

FGameplayCueParameters AGeoDeployableBase::GetRecallCueParams() const
{
	FGameplayCueParameters CueParams = GetGenericCueParams(RecallCue);
	AActor const* CueInstigator = GetData()->Instigator;
	CueParams.Normal = IsValid(CueInstigator) ? (CueInstigator->GetActorLocation() - GetActorLocation()).GetSafeNormal()
											  : FVector::ZeroVector;
	CueParams.NormalizedMagnitude = IsBlinking() ? 1.f : 0.f;
	return CueParams;
}
