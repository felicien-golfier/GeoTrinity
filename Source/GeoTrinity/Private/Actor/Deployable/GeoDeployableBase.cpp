// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableBase.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Component/GeoCombattantWidgetComp.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = GetDefault<UGameDataSettings>()->RegularTickInterval;
	HealthBarComponent = CreateDefaultSubobject<UGeoCombattantWidgetComp>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);
	HealthBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);

	if (TSubclassOf<UUserWidget> const HealthBarWidgetClass =
			GetDefault<UGameDataSettings>()->DefaultDeployableHealthBarWidgetClass.LoadSynchronous())
	{
		HealthBarComponent->SetWidgetClass(HealthBarWidgetClass);
	}

	SetReplicates(true);
}

void AGeoDeployableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGeoDeployableBase, bActive);
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
		DrainEffectData.bSuppressGameplayCue =
			bSuppressDrainDamageVisuals ? true : !GameFeelComponent->IsDamageCueAvailable();
		UGeoAbilitySystemLibrary::ApplySingleEffectData(DrainEffectData, ASC, ASC, GetData()->Level, GetData()->Seed);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::BeginPlay()
{
	Super::BeginPlay();

	if (UGeoDeployableManagerComponent* DeployableManager =
			GetInstigator()->GetComponentByClass<UGeoDeployableManagerComponent>())
	{
		DeployableManager->RegisterDeployable(this);
	}

	InitDrain();

	if (!CanBeDamaged())
	{
		HealthBarComponent->SetHiddenInGame(true);
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
	ExecuteRecallCue();
	Expire();
}


// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::ExecuteRecallCue()
{
	if (!RecallGameplayCueTag.IsValid())
	{
		return;
	}

	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ensureMsgf(IsValid(ASC), TEXT("AGeoDeployableBase: no ASC on self")))
	{
		return;
	}

	ASC->ExecuteGameplayCue(RecallGameplayCueTag, GetRecallCueParams());
}

void AGeoDeployableBase::RecallEffect(float Value)
{
	// Override to add effects.
}

void AGeoDeployableBase::Explode(float Value)
{
	UGeoAbilitySystemComponent* SourceASC = GeoASLib::GetGeoAscFromActor(GetData()->Owner);
	if (!ensureMsgf(SourceASC, TEXT("AGeoMine: no ASC on Owner")))
	{
		return;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_Pawn),
														 UEngineTypes::ConvertToObjectType(ECC_GeoCharacter)};
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

		ExplodeEffect(Value, SourceASC, Actor, ActorASC);
	}
}

void AGeoDeployableBase::ExplodeEffect(float Value, UGeoAbilitySystemComponent* SourceASC, AActor* Actor,
									   UGeoAbilitySystemComponent* TargetASC)
{
	GeoASLib::ApplyEffectFromEffectData(GetData()->EffectDataArray, SourceASC, TargetASC, GetData()->Level,
										GetData()->Seed);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Expire()
{
	bActive = false;
	GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BlinkVisibilityTimerHandle);
	SetActorHiddenInGame(true);
	OnDeployableExpiredEvent.Broadcast(this);
	SetActorTickEnabled(false);

	if (TimeBeforeDestroyAtExpire > 0.f)
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
			TimeBeforeDestroyAtExpire, false);
	}
	else
	{
		Destroy();
	}
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

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnHealthChanged_Implementation(float NewValue)
{
	if (NewValue <= 0.f && bActive && !BlinkTimerHandle.IsValid())
	{
		float const BlinkDuration = GetData()->Params.BlinkDuration;
		if (BlinkDuration > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(BlinkTimerHandle, this, &ThisClass::OnBlinkTimerExpired,
												   BlinkDuration, false);
			OnBlinkStarted();
			SetActorEnableCollision(false);
		}
		else
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
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkStarted_Implementation()
{
	float constexpr BlinkRate = 0.2f;
	GetWorld()->GetTimerManager().SetTimer(BlinkVisibilityTimerHandle, this, &ThisClass::OnBlinkVisibilityTick,
										   BlinkRate, true);
}

void AGeoDeployableBase::OnRep_Expired(bool bOldValue)
{
	if (bOldValue && !bActive)
	{
		ExecuteRecallCue();
		Expire();
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkVisibilityTick()
{
	SetActorHiddenInGame(!IsHidden());
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkTimerExpired()
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
FGameplayCueParameters AGeoDeployableBase::GetRecallCueParams()
{
	FVector const DeployableLocation = GetActorLocation();

	FGameplayCueParameters CueParams;
	CueParams.Location = DeployableLocation;
	CueParams.Normal = (GetData()->Instigator->GetActorLocation() - DeployableLocation).GetSafeNormal();
	CueParams.EffectCauser = this;
	CueParams.Instigator = GetData()->Instigator;
	CueParams.AbilityLevel = GetData()->Level;
	CueParams.RawMagnitude = IsBlinking() ? 1.f : 0.f;
	return CueParams;
}
