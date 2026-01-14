// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/GeoInteractableActor.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"

// Sets default values
AGeoInteractableActor::AGeoInteractableActor()
{
	bReplicates = true;   // Directly setting bReplicates is the correct procedure for pre-init actors.
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSetBase = CreateDefaultSubobject<UGeoAttributeSetBase>(TEXT("AttributeSetBase"));

	SetNetUpdateFrequency(100.f);
}

void AGeoInteractableActor::InitInteractableData(FInteractableActorData* Data)
{
	check(Data);
	InteractableActorData = Data;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	InitGas();
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoInteractableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindGasCallbacks();

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------------------------------------------------
UAbilitySystemComponent* AGeoInteractableActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoInteractableActor::BindGasCallbacks()
{
	checkf(AbilitySystemComponent,
		TEXT("Trying to bind GAS callbacks but AbilitySystemComponent is null. This is too early ?"));
	AbilitySystemComponent->OnHealthChanged.AddDynamic(this, &ThisClass::OnHealthChanged);
	AbilitySystemComponent->OnMaxHealthChanged.AddDynamic(this, &ThisClass::OnMaxHealthChanged);
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoInteractableActor::UnbindGasCallbacks()
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
void AGeoInteractableActor::OnHealthChanged(float NewValue)
{
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoInteractableActor::OnMaxHealthChanged(float NewValue)
{
}

void AGeoInteractableActor::ApplyEffectToSelf_Implementation(TSubclassOf<UGameplayEffect> gameplayEffectClass,
	float level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	EffectContextHandle.AddInstigator(InteractableActorData->CharacterOwner, this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(gameplayEffectClass, level, EffectContextHandle);

	if (SpecHandle.IsValid())
	{
		FPredictionKey PredictionKey;
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ASC, PredictionKey);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void AGeoInteractableActor::InitGas()
{
	check(AbilitySystemComponent);

	// TODO: Shouldn't we give the OwnerActor From the InteractableData instead of itself ?
	// TODO: And so call the InitGas in the InitInteractableData ?
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->InitializeDefaultAttributes();

	if (HasAuthority())
	{
		AbilitySystemComponent->GiveStartupAbilities();
	}

	BindGasCallbacks();
}
