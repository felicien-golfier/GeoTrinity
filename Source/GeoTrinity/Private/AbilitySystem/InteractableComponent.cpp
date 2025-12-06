// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/InteractableComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoTrinity/GeoTrinity.h"

// Sets default values for this component's properties
UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	UE_VLOG(this, LogGeoASC, VeryVerbose, TEXT("UInteractableComponent::UInteractableComponent(), role %s"),
		*UEnum::GetValueAsString(GetOwnerRole()));

	//if (GetOwner())
	//{
	//	UE_LOG(LogGeoASC, Log, TEXT("UInteractableComponent [%s] has an owner with a role %s"), *GetName(), *UEnum::GetValueAsString(GetOwnerRole()));
	//	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	//	AbilitySystemComponent->SetIsReplicated(true);
	//	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	//}	
}

// ---------------------------------------------------------------------------------------------------------------------
UAttributeSet* UInteractableComponent::GetAttributeSet() const
{
	return AttributeSet;
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (AActor* Owner = GetOwner())
	{
		if (!bInitGasAtBeginPlay)
		{
			return;
		}
		
		// Create ASC, and set it to be explicitly replicated
		AbilitySystemComponent = NewObject<UGeoAbilitySystemComponent>(Owner, TEXT("AbilitySystemComponent"));
		AbilitySystemComponent->RegisterComponent();
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
		
		if (!AttributeSet)
		{
			AttributeSet = NewObject<UGeoAttributeSetBase>(Owner);
			AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());
		}
		
		InitGas(
			AbilitySystemComponent,
			GetOwner(), 
			AttributeSet);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GROS PROBLEM"));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::InitGas(UGeoAbilitySystemComponent* GeoAbilitySystemComponent, AActor* OwnerActor,
	UGeoAttributeSetBase* NewAttributeSet)
{
	InitAbilityActorInfo(GeoAbilitySystemComponent, OwnerActor, NewAttributeSet);

	if (GetOwner()->HasAuthority())
	{
		InitializeDefaultAttributes();
		AddCharacterDefaultAbilities();		
	}
	
	// Bind for cosmetic and gameplay purposes, so both on client and on server
	BindGasCallbacks();
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::InitAbilityActorInfo(UGeoAbilitySystemComponent* GeoAbilitySystemComponent,
	AActor* OwnerActor, UGeoAttributeSetBase* NewAttributeSet)
{
	AbilitySystemComponent = GeoAbilitySystemComponent;
	AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, GetOwner());
	//if (IsValid(NewAttributeSet))
	//{
	//	AttributeSet = NewAttributeSet;
	//	AbilitySystemComponent->AddSpawnedAttribute(AttributeSet);
	//}
	
	if (!IsValid(AttributeSet) && IsValid(NewAttributeSet))
	{
		AttributeSet = NewAttributeSet;
		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::InitializeDefaultAttributes() const
{
	check(IsValid(AbilitySystemComponent));

	if (!IsValid(DefaultAttributes))
	{
		UE_LOG(LogGeoTrinity, Error,
			TEXT("%s() Missing DefaultAttributes for %s. Please fill in the pawn's Blueprint."), *FString(__FUNCTION__),
			*GetName());
		return;
	}

	ApplyEffectToSelf(DefaultAttributes, Level);

	UE_LOG(LogTemp, Log, TEXT("[%s] After initialization, my health attributes are : Health=%f / MaxHealth=%f"),
		*GetOwner()->GetName(), AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::AddCharacterDefaultAbilities() const
{
	checkf(AbilitySystemComponent, TEXT("%s() AbilitySystemComponent is null. Did we call this too soon ?"),
		*FString(__FUNCTION__));

	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogGeoASC, Warning,
			TEXT("This should not be the case, as only the server should be calling this method"));
	}
	AbilitySystemComponent->AddCharacterStartupAbilities(StartupAbilityTags, Level);
}

// ---------------------------------------------------------------------------------------------------------------------
void UInteractableComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> gameplayEffectClass, float level) const
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
		.AddWeakLambda(this,
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(Data.NewValue);
			});
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute())
		.AddWeakLambda(this,
			[this](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(Data.NewValue);
			});
}