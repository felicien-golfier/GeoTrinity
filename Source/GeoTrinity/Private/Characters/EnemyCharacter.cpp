// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/EnemyCharacter.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(
		  ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Create ASC, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// Adding an attribute set as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	AttributeSetBase = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("AttributeSetBase"));
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitAbilityActorInfo();
}

void AEnemyCharacter::InitAbilityActorInfo()
{
	// DO NOT CALL SUPER !
	AGeoCharacter::InitAbilityActorInfo(AbilitySystemComponent, this, AttributeSetBase);
}

void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}