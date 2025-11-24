// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Turret/GeoTurretBase.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "Components/CapsuleComponent.h"
#include "GeoTrinity/GeoTrinity.h"


// ---------------------------------------------------------------------------------------------------------------------
AGeoTurretBase::AGeoTurretBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoCapsule"));
	
	ASC = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	AttributeSet = CreateDefaultSubobject<UGeoAttributeSetBase>(TEXT("AttributeSet"));

	SetNetUpdateFrequency(100.0f);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::InitTurretData(TurretInitData const& data)
{
	TurretLevel = data.TurretLevel;
	CharacterOwner = data.CharacterOwner;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::BeginPlay()
{
	Super::BeginPlay();
	
	ASC->InitAbilityActorInfo(this, this);
	
	InitializeDefaultAttributes();
	AddDefaultAbilities();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::InitializeDefaultAttributes()
{
	check(IsValid(ASC));

	if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("%s() Missing DefaultAttributes for %s. Please fill in the Blueprint."), *FString(__FUNCTION__),
			*GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, TurretLevel);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::AddDefaultAbilities()
{
	ASC->AddCharacterStartupAbilities(StartupAbilities);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::ApplyEffectToSelf_Implementation(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level)
{
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	EffectContextHandle.AddInstigator(CharacterOwner.Get(), this);

	const FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(gameplayEffectClass, level, EffectContextHandle);

	if (SpecHandle.IsValid())
	{
		FPredictionKey PredictionKey;
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ASC,
			PredictionKey);
	}
}