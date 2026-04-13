// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableBase.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "HUD/Component/GeoCombattantWidgetComp.h"
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

	// Based on the GameDataSettings RegularTickInterval frequence
	if (bUseRegularDrain && GeoLib::IsServer(GetWorld()))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		FDamageEffectData DrainEffectData = FDamageEffectData();
		DrainEffectData.DamageAmount = DrainMagnitudePerSecond * DeltaSeconds;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(DrainEffectData, ASC, ASC, GetData()->Level, GetData()->Seed);
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

void AGeoDeployableBase::BeginPlay()
{
	Super::BeginPlay();
	InitDrain();
}
// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::Recall(float Value)
{
	GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
	OnDeployableExpired();
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
	if (NewValue <= 0.f && !bExpired && !BlinkTimerHandle.IsValid())
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
			OnDeployableExpired();
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

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkVisibilityTick()
{
	SetActorHiddenInGame(!IsHidden());
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnBlinkTimerExpired()
{
	OnDeployableExpired();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnDeployableExpired_Implementation()
{
	if (bExpired)
	{
		return;
	}
	bExpired = true;
	GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BlinkVisibilityTimerHandle);
	SetActorHiddenInGame(true);
	OnDeployableDestroyed.Broadcast(this);
	Destroy();
}


bool AGeoDeployableBase::IsBlinking() const
{
	return BlinkTimerHandle.IsValid();
}
