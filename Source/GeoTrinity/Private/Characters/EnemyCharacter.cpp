// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/EnemyCharacter.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UGeoCharacterMovementComponent>(CharacterMovementComponentName))
{
	// Ensure an AI controller and auto possession for enemies
	AIControllerClass = AGeoEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	TeamId = ETeam::Enemy;

	// Create ASC, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// Adding an attribute set as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	AttributeSetBase = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("AttributeSetBase"));

	SetNetUpdateFrequency(100.f);
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitGAS();
}

void AEnemyCharacter::InitGAS()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Super::InitGAS();

	AbilitySystemComponent->OnHealthChanged.AddDynamic(
		this,
		&AEnemyCharacter::OnHealthChanged); // Do we need to remove this on destroy?
}

void AEnemyCharacter::OnHealthChanged_Implementation(float NewValue)
{
	if (HasAuthority() && NewValue <= 0.f)
	{
		if (ResetToFullLifeWhenReachingZero)
		{
			// Modify Health using an effect on the fly

			UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage());
			GE->DurationPolicy = EGameplayEffectDurationType::Instant;

			// 2. Define the modifier (definition-level)
			FGameplayModifierInfo ModifierInfo;
			ModifierInfo.Attribute = UGeoAttributeSetBase::GetHealthAttribute();
			ModifierInfo.ModifierOp = EGameplayModOp::Override;
			UGeoAttributeSetBase const* AS = Cast<UGeoAttributeSetBase>(
				AbilitySystemComponent->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
			ModifierInfo.ModifierMagnitude = FScalableFloat(AS->GetMaxHealth());

			GE->Modifiers.Add(ModifierInfo);

			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(AbilitySystemComponent->GetAvatarActor());

			FGameplayEffectSpec const Spec(GE, Context, 1.f);

			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(Spec);
		}
		else
		{
			AbilitySystemComponent->StopAllActivePatterns();
			Destroy();
		}
	}
}
