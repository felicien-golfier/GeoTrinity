// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableBase.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "HUD/Interface/GeoCombattantWidgetHost.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoShape"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = GetDefault<UGameDataSettings>()->RegularTickInterval;

	WidgetAnchorComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WidgetAnchorComponent"));
	WidgetAnchorComponent->SetupAttachment(GetRootComponent());
	WidgetAnchorComponent->SetUsingAbsoluteRotation(true);

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
	if (bPushActorsOnSpawn)
	{
		PushAway();
	}
}

void AGeoDeployableBase::PushAway()
{
	FVector2D const Location2D(GetActorLocation());
	float const Radius = CapsuleComponent->GetScaledCapsuleRadius();

	for (AActor* Actor : GeoASLib::GetInteractableActors(this, true, Location2D, Radius))
	{
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (!IsValid(Character))
		{
			continue;
		}

		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		if (!IsValid(Movement))
		{
			continue;
		}

		FVector PushDirection = (Actor->GetActorLocation() - GetActorLocation());
		PushDirection.Z = 0.f;
		if (PushDirection.IsNearlyZero())
		{
			PushDirection = FVector(1.f, 0.f, 0.f);
		}
		PushDirection.Normalize();

		constexpr float PushDuration = 0.15f;
		FVector const PushTarget =
			Actor->GetActorLocation() + PushDirection * (Radius + Actor->GetSimpleCollisionRadius());

		TSharedPtr<FRootMotionSource_MoveToForce> PushRootMotion = MakeShared<FRootMotionSource_MoveToForce>();
		PushRootMotion->InstanceName = TEXT("PillarPush");
		PushRootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
		PushRootMotion->StartLocation = Actor->GetActorLocation();
		PushRootMotion->TargetLocation = PushTarget;
		PushRootMotion->Duration = PushDuration;
		PushRootMotion->bRestrictSpeedToExpected = false;
		PushRootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
		PushRootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

		uint16 const SourceID = Movement->ApplyRootMotionSource(PushRootMotion);

		FTimerHandle RemoveHandle;
		GetWorldTimerManager().SetTimer(
			RemoveHandle,
			[Movement, SourceID]()
			{
				if (IsValid(Movement))
				{
					Movement->RemoveRootMotionSourceByID(SourceID);
					Movement->SetMovementMode(MOVE_Falling);
				}
			},
			PushDuration, false);
	}
}

void AGeoDeployableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGeoDeployableBase, bActive);
	DOREPLIFETIME(AGeoDeployableBase, bBlinking);
}

void AGeoDeployableBase::InitDrain()
{
	if (!HasAuthority() || GetData()->Params.LifeDrainMaxDuration <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f)
	{
		ensureMsgf(MaxHealth > 0.f, TEXT("AGeoDeployableBase: MaxHealth is 0 — DefaultAttributes may not be applied."));
		return;
	}

	DrainMagnitudePerSecond = MaxHealth / GetData()->Params.LifeDrainMaxDuration;
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bUseRegularDrain && GeoLib::IsServer(GetWorld()))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		FDamageEffectData DrainEffectData;
		DrainEffectData.DamageAmount = DrainMagnitudePerSecond * DeltaSeconds;
		DrainEffectData.bSuppressGameplayCue = bSuppressDrainDamageVisuals;
		DrainEffectData.bLimitGameplayCue = true;
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

void AGeoDeployableBase::BeginPlay()
{
	Super::BeginPlay();

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

	ExecuteCue(SpawnGameplayCueTag, GetGenericCueParams(SpawnSoundTag));
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Recall(float Value)
{
	if (!bActive)
	{
		return;
	}
	bActive = false;

	RecallEffect(Value);
	// Local cue: run on every rendering machine incl. the listen-server host; skip only the dedicated server.
	if (!GeoLib::IsDedicatedServer(GetWorld()) && RecallGameplayCueTag.IsValid())
	{
		ExecuteCue(RecallGameplayCueTag, GetRecallCueParams());
	}
	Expire();
}


// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::ExecuteCue(FGameplayTag const& GameplayCueTag, FGameplayCueParameters const& CueParams) const
{
	if (!GameplayCueTag.IsValid())
	{
		return;
	}

	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!IsValid(ASC))
	{
		ensureMsgf(IsValid(ASC), TEXT("AGeoDeployableBase: no ASC on self"));
		return;
	}

	// Local-only: each machine fires the cue once (host via the call site, clients via OnRep). ExecuteGameplayCue would
	// multicast on the authority, double-playing it on clients with engine-stripped params.
	ASC->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed, CueParams);
}

void AGeoDeployableBase::RecallEffect(float Value)
{
	if (bExplodeAtRecall)
	{
		Explode(Value);
	}
}

void AGeoDeployableBase::Explode(float Value)
{
	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(GetData()->Owner);
	if (!ensureMsgf(SourceASC, TEXT("AGeoDeployableBase: no ASC on Owner")))
	{
		return;
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		TArray<AActor*> OverlappingActors =
			GeoASLib::GetInteractableActors(this, GeoASLib::GetTeamId(GetData()->Owner), ExplodeAttitude, true,
											FVector2D(GetActorLocation()), GetData()->Params.Size);

		for (AActor* Actor : OverlappingActors)
		{
			UGeoAbilitySystemComponent* ActorASC = GeoASLib::GetGeoAscFromActor(Actor);
			if (!IsValid(ActorASC))
			{
				continue;
			}

			ApplyExplodeEffect(Value, SourceASC, Actor, ActorASC);
		}
	}

	if (!GeoLib::IsDedicatedServer(this) && ExplodeGameplayCueTag.IsValid())
	{
		ExecuteCue(ExplodeGameplayCueTag, GetGenericCueParams(ExplodeSoundTag));
		// TODO: Pass the Value in the Cue ?
	}
}

void AGeoDeployableBase::ApplyExplodeEffect(float Value, UGeoAbilitySystemComponent* SourceASC, AActor* Actor,
											UGeoAbilitySystemComponent* TargetASC)
{
	GeoASLib::ApplyEffectFromEffectData(GetData()->EffectDataArray, SourceASC, TargetASC, GetData()->Level,
										GetData()->Seed, GetData()->AbilityTag);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Expire(float const TimeBeforeDestroy)
{
	bActive = false;
	GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BlinkVisibilityTimerHandle);
	SetActorHiddenInGame(true);
	OnDeployableExpiredEvent.Broadcast(this);
	SetActorTickEnabled(false);

	if (TimeBeforeDestroy > 0.f)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			[this]()
			{
				if (IsValid(this))
				{
					this->Destroy();
				}
			},
			TimeBeforeDestroy, false);
	}
	else
	{
		Destroy();
	}
}

void AGeoDeployableBase::Expire()
{
	Expire(TimeBeforeDestroyAtExpire);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float AGeoDeployableBase::GetDurationPercent() const
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

void AGeoDeployableBase::StartBlinking(float const BlinkDuration)
{
	bBlinking = true;
	GetWorld()->GetTimerManager().SetTimer(BlinkTimerHandle, this, &ThisClass::TryRecallOrExpire, BlinkDuration, false);

	// Local cue: run on every rendering machine incl. the listen-server host; skip only the dedicated server.
	if (BlinkingGameplayCueTag.IsValid() && !GeoLib::IsDedicatedServer(this))
	{
		FGameplayCueParameters CueParams = GetGenericCueParams(BlinkingSoundTag);
		CueParams.Normal = FVector(BlinkDuration, 0.f, 0.f);
		ExecuteCue(BlinkingGameplayCueTag, CueParams);
	}

	OnBlinkVisualStarted();
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
}
// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnHealthChanged_Implementation(float NewValue)
{
	if (NewValue <= 0.f && bActive && !BlinkTimerHandle.IsValid())
	{
		float const BlinkDuration = GetData()->Params.BlinkDuration;
		if (BlinkDuration > 0.f)
		{
			StartBlinking(BlinkDuration);
		}
		else
		{
			TryRecallOrExpire();
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkVisualStarted_Implementation()
{
	float constexpr BlinkRate = 0.2f;
	GetWorld()->GetTimerManager().SetTimer(BlinkVisibilityTimerHandle, this, &ThisClass::OnBlinkVisibilityTick,
										   BlinkRate, true);
}

void AGeoDeployableBase::OnRep_Active(bool bOldValue)
{
	if (bOldValue && !bActive)
	{
		ExecuteCue(RecallGameplayCueTag, GetRecallCueParams());
		if (bExplodeAtRecall)
		{
			ExecuteCue(ExplodeGameplayCueTag, GetGenericCueParams(ExplodeSoundTag));
		}
		Expire();
	}
}
void AGeoDeployableBase::OnRep_Blinking(bool bOldValue)
{
	if (!bOldValue && bBlinking && GetData()->Params.BlinkDuration > 0.f)
	{
		StartBlinking(GetData()->Params.BlinkDuration);
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
	return BlinkTimerHandle.IsValid();
}


// ---------------------------------------------------------------------------------------------------------------------

FGameplayCueParameters AGeoDeployableBase::GetGenericCueParams(FGameplayTag SoundTag)
{
	FGameplayCueParameters CueParams;
	CueParams.Location = GetActorLocation();
	// TODO: find a better solution
	CueParams.Location.Z = 1.f; // Ensure all Cues happens just above the floor
	CueParams.EffectCauser = this;
	CueParams.Instigator = GetData()->Instigator;
	CueParams.AbilityLevel = GetData()->Level;
	CueParams.RawMagnitude = GetData()->Params.Size;
	CueParams.NormalizedMagnitude = GetData()->Params.Value;
	if (SoundTag.IsValid())
	{
		CueParams.AggregatedSourceTags.AddTag(SoundTag);
	}
	return CueParams;
}

FGameplayCueParameters AGeoDeployableBase::GetRecallCueParams()
{
	FGameplayCueParameters CueParams = GetGenericCueParams(RecallSoundTag);
	CueParams.Normal = (GetData()->Instigator->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	CueParams.NormalizedMagnitude = IsBlinking() ? 1.f : 0.f;
	return CueParams;
}
