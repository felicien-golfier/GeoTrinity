// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/Turret/GeoTurretBase.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/InteractableComponent.h"
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

	SetNetUpdateFrequency(100.0f);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::InitTurretData(TurretInitData const& data)
{
	TurretData = data;

	//if (UInteractableComponent* InteractableComponent = GetComponentByClass<UInteractableComponent>())
	//{
	//	InteractableComponent->SetLevel(TurretData.TurretLevel);
	//}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoTurretBase::BeginPlay()
{
	Super::BeginPlay();
}

// ---------------------------------------------------------------------------------------------------------------------
UAbilitySystemComponent* AGeoTurretBase::GetAbilitySystemComponent() const
{
	if (UInteractableComponent* InteractableComponent = GetComponentByClass<UInteractableComponent>())
	{
		return InteractableComponent->AbilitySystemComponent;
	}
	UE_LOG(LogTemp, Warning, TEXT("Do we expect NOT to have an InteractableComponent on %s ?"), *GetName());
	return nullptr;
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