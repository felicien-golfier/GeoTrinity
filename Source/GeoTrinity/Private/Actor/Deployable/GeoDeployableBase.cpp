// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableBase.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase()
{
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoShape"));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = GetDefault<UGameDataSettings>()->RegularTickInterval;
	// CombattantWidgetComponent (the world-space health bar) is a UGeoCombattantWidgetComp added in Blueprint — it
	// lives in the UI module which gameplay must not reference. It is resolved and configured from the BP component in
	// BeginPlay. The BP attaches it to a non-rotating anchor so the bar's offset doesn't orbit the deployable as it
	// yaws.
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

	// The health-bar widget component is added in Blueprint (a UGeoCombattantWidgetComp from the UI module). Resolve it
	// as the engine base; apply the project default widget class if the BP left it unset (engine base API only).
	CombattantWidgetComponent = FindComponentByClass<UWidgetComponent>();
	// A damageable deployable must show a health bar, so its BP must include the widget component. Flag a missing one
	// (the dedicated server is headless and never renders it, so skip the check there).
	ensureMsgf(CombattantWidgetComponent || !CanBeDamaged() || GeoLib::IsDedicatedServer(GetWorld()),
			   TEXT("%s is damageable but has no UGeoCombattantWidgetComp — add one in its Blueprint"), *GetName());
	if (CombattantWidgetComponent && !CombattantWidgetComponent->GetWidgetClass())
	{
		if (TSubclassOf<UUserWidget> const HealthBarWidgetClass =
				GetDefault<UGameDataSettings>()->DefaultDeployableHealthBarWidgetClass.LoadSynchronous())
		{
			CombattantWidgetComponent->SetWidgetClass(HealthBarWidgetClass);
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
	if (!GeoLib::IsDedicatedServer(GetWorld()))
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

	FScopedPredictionWindow ScopedPredictionWindow(ASC);
	ASC->ExecuteGameplayCue(GameplayCueTag, CueParams);
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
	if (!ensureMsgf(SourceASC, TEXT("AGeoMine: no ASC on Owner")))
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
	else
	{
		if (ExplodeGameplayCueTag.IsValid())
		{
			ExecuteCue(ExplodeGameplayCueTag, GetGenericCueParams());
			// TODO: Pass the Value in the Cue ?
		}
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
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		FScopedPredictionWindow ScopedPredictionWindow(ASC);
		FGameplayCueParameters CueParams = GetGenericCueParams();
		CueParams.Normal = FVector(BlinkDuration, 0.f, 0.f);
		ASC->ExecuteGameplayCue(BlinkingGameplayCueTag, CueParams);
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
			ExecuteCue(ExplodeGameplayCueTag, GetGenericCueParams());
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
FGameplayCueParameters AGeoDeployableBase::GetGenericCueParams()
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

	return CueParams;
}

FGameplayCueParameters AGeoDeployableBase::GetRecallCueParams()
{
	FGameplayCueParameters CueParams = GetGenericCueParams();
	CueParams.Normal = (GetData()->Instigator->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	CueParams.NormalizedMagnitude = IsBlinking() ? 1.f : 0.f;
	return CueParams;
}
