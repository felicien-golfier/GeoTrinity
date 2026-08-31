// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/EnemyCharacter.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "Engine/World.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/Character.h"
#include "Tool/UGeoGameplayLibrary.h"

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
	bAlwaysRelevant = true;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitGAS();
}

void AEnemyCharacter::InitGAS()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Super grants the abilities and applies the attributes, so the level has to be in place first.
	AbilitySystemComponent->SetCombatLevel(
		static_cast<int32>(GetWorld()->GetGameStateChecked<AGeoGameState>()->GetDifficulty()));

	Super::InitGAS();

	AbilitySystemComponent->OnHealthChanged.AddDynamic(
		this,
		&AEnemyCharacter::OnHealthChanged); // Do we need to remove this on destroy?
}

void AEnemyCharacter::OnHealthChanged_Implementation(float NewValue)
{
	if (GeoLib::IsServer(this) && NewValue <= 0.f)
	{
		if (ResetToFullLifeWhenReachingZero)
		{
			AbilitySystemComponent->InitializeDefaultAttributes();
		}
		else
		{
			OnEnemyDefeated.Broadcast();
			Destroy();
		}
	}
}

bool AEnemyCharacter::IsFighting() const
{
	return Arena.IsValid() && Arena->IsFighting();
}

void AEnemyCharacter::ResetForNewAttempt()
{
	StopAllSpawnedElements();
	AbilitySystemComponent->InitializeDefaultAttributes();
	if (AGeoEnemyAIController* AIController = GetController<AGeoEnemyAIController>())
	{
		AIController->ResetAI();
	}
}
