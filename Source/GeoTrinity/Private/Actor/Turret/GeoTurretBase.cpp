// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Turret/GeoTurretBase.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "Components/CapsuleComponent.h"


// ---------------------------------------------------------------------------------------------------------------------
AGeoTurretBase::AGeoTurretBase()
{
	bReplicates = true;	//Directly setting bReplicates is the correct procedure for pre-init actors.
	PrimaryActorTick.bCanEverTick = false;
	
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoCapsule"));

	
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSetBase = CreateDefaultSubobject<UGeoAttributeSetBase>(TEXT("AttributeSetBase"));
	
	SetNetUpdateFrequency(100.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::InitTurretData(TurretInitData const& data)
{
	TurretData = data;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilityActorInfo();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindGasCallbacks();
	
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
UAbilitySystemComponent* AGeoTurretBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::BindGasCallbacks()
{
	checkf(AbilitySystemComponent, TEXT("Trying to bind GAS callbacks but AbilitySystemComponent is null. This is too early ?"));
	AbilitySystemComponent->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
	AbilitySystemComponent->OnMaxHealthChanged.AddDynamic(this, &ThisClass::OnMaxHealthChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::UnbindGasCallbacks()
{
	if (!AbilitySystemComponent)
	{
		// It's already been invalidated, so don't try to unbind
		return;
	}
	AbilitySystemComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::OnHealthChanged);
	AbilitySystemComponent->OnMaxHealthChanged.RemoveDynamic(this, &ThisClass::OnMaxHealthChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::OnHealthChanged(float NewValue)
{
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::OnMaxHealthChanged(float NewValue)
{
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::ApplyEffectToSelf_Implementation(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	EffectContextHandle.AddInstigator(TurretData.CharacterOwner, this);

	const FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(gameplayEffectClass, level, EffectContextHandle);

	if (SpecHandle.IsValid())
	{
		FPredictionKey PredictionKey;
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ASC,
			PredictionKey);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::InitAbilityActorInfo()
{
	check(AbilitySystemComponent)
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	if (HasAuthority())
	{
		AbilitySystemComponent->InitializeDefaultAttributes();
		AbilitySystemComponent->AddCharacterStartupAbilities();
	}
	
	BindGasCallbacks();
}