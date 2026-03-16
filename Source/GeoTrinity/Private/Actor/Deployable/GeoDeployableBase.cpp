// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableBase.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "HUD/Component/GeoCombattantWidgetComp.h"
#include "Settings/GameDataSettings.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
AGeoDeployableBase::AGeoDeployableBase()
{
	PrimaryActorTick.bCanEverTick = false;

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

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || GetData()->MaxDuration <= 0.f)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> const HealthDrainEffect =
		GetDefault<UGameDataSettings>()->DeployableHealthDrainEffect.LoadSynchronous();
	ensureMsgf(HealthDrainEffect,
			   TEXT("AGeoDeployableBase: DeployableHealthDrainEffect is not set in GameDataSettings!"));
	if (!HealthDrainEffect)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	float const MaxHealth = ASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	ensureMsgf(MaxHealth > 0.f, TEXT("AGeoDeployableBase: MaxHealth is 0 — DefaultAttributes may not be applied."));
	if (MaxHealth <= 0.f)
	{
		return;
	}
	float const DrainPerSecond = MaxHealth / GetData()->MaxDuration;
	float const DrainPeriod = HealthDrainEffect.GetDefaultObject()->Period.Value;
	FGameplayEffectSpecHandle const SpecHandle =
		ASC->MakeOutgoingSpec(HealthDrainEffect, GetData()->Level, ASC->MakeEffectContext());
	SpecHandle.Data->SetSetByCallerMagnitude(FGeoGameplayTags::Get().Data_Drain, -(DrainPerSecond * DrainPeriod));
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnRecalled()
{
	GetWorld()->GetTimerManager().ClearTimer(BlinkTimerHandle);
	OnDeployableExpired();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float AGeoDeployableBase::GetDurationPercent() const
{
	if (GetData()->MaxDuration <= 0.f)
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
void AGeoDeployableBase::InitInteractableData(FInteractableActorData* InputData)
{
	Super::InitInteractableData(InputData);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void AGeoDeployableBase::OnHealthChanged(float NewValue)
{
	if (NewValue <= 0.f && !bExpired && !BlinkTimerHandle.IsValid())
	{
		float const BlinkDuration = GetData()->BlinkDuration;
		if (BlinkDuration > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(BlinkTimerHandle, this, &ThisClass::OnBlinkTimerExpired,
												   BlinkDuration, false);
			OnBlinkStarted();
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
void AGeoDeployableBase::OnDeployableExpired()
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
