// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/InteractableComponent.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoTrinity/GeoTrinity.h"

// Sets default values for this component's properties
UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Create ASC, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSetBase = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("AttributeSetBase"));
}

// ---------------------------------------------------------------------------------------------------------------------
UAttributeSet* UInteractableComponent::GetAttributeSetBase() const
{
	return AttributeSetBase;
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bInitGasAtBeginPlay)
	{
		InitGas(AbilitySystemComponent, GetOwner(), AttributeSetBase);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::InitGas(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
	UCharacterAttributeSet* GeoAttributeSetBase)
{
	InitAbilityActorInfo(GeoAbilitySystemComponent, OwnerActor, GeoAttributeSetBase);
	
	if (GetOwner()->HasAuthority())
	{
		InitializeDefaultAttributes();
		
		BindGasCallbacks();
		AddCharacterDefaultAbilities();
	}
	
	// fake
	OnMaxHealthChanged.Broadcast(100.f);
	OnHealthChanged.Broadcast(90.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::InitAbilityActorInfo(UGeoAbilitySystemComponent* GeoAbilitySystemComponent,
	AActor* OwnerActor, UCharacterAttributeSet* GeoAttributeSetBase)
{
	AbilitySystemComponent = GeoAbilitySystemComponent;
	AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, GetOwner());
	AttributeSet = GeoAttributeSetBase;
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::InitializeDefaultAttributes()
{
	check(IsValid(AbilitySystemComponent));

	if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("%s() Missing DefaultAttributes for %s. Please fill in the pawn's Blueprint."), *FString(__FUNCTION__),
			*GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, 1.0f);

	UE_LOG(LogTemp, Log, TEXT("[%s] After initialization, my health attributes are : Health=%f / MaxHealth=%f"), *GetOwner()->GetName(),
		AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::AddCharacterDefaultAbilities()
{
	checkf(AbilitySystemComponent, TEXT("%s() AbilitySystemComponent is null. Did we call this too soon ?"),
		*FString(__FUNCTION__));

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogGeoASC, Warning,
			TEXT("This should not be the case, as only the server should be calling this method"));
	}
	AbilitySystemComponent->AddCharacterStartupAbilities(StartupAbilities);
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::BP_ApplyEffectToSelfDefaultLvl(TSubclassOf<UGameplayEffect> gameplayEffectClass)
{
	ApplyEffectToSelf(gameplayEffectClass, 1.0f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::ApplyEffectToSelf_Implementation(TSubclassOf<UGameplayEffect> gameplayEffectClass,
	float level)
{
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(gameplayEffectClass, level, EffectContextHandle);

	if (SpecHandle.IsValid())
	{
		FPredictionKey PredictionKey;
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent.Get(),
			PredictionKey);
		// AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
	}
}

ETeamAttitude::Type UInteractableComponent::GetTeamAttitudeTowards(const AActor& Other) const
{

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	return OtherTeamAgent->GetGenericTeamId() == GetGenericTeamId() ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void UInteractableComponent::BindGasCallbacks()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
			.AddWeakLambda(this, [this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.NewValue);
			});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute())
			.AddWeakLambda(this, [this](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			});
}