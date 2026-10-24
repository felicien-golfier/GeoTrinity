// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/EnemyCharacter.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "Characters/Component/FX/GeoLoopSoundComponent.h"
#include "Characters/GeoEnemySetup.h"
#include "Engine/World.h"
#include "GameClasses/Match/GeoGameState.h"
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

	LoopSoundComponent = CreateDefaultSubobject<UGeoLoopSoundComponent>(TEXT("LoopSoundComponent"));

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

	ApplyMaxHealthOverride();
	AbilitySystemComponent->SetNumericAttributeBase(
		UGeoAttributeSetBase::GetHealthAttribute(),
		AbilitySystemComponent->GetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute())
			* StartHealthRatio);

	AbilitySystemComponent->OnHealthChanged.AddDynamic(
		this,
		&AEnemyCharacter::OnHealthChanged); // Do we need to remove this on destroy?
}

void AEnemyCharacter::ApplySetup(FGeoEnemySetup const& Setup)
{
	TeamId = Setup.Team;
	ResetToFullLifeWhenReachingZero = Setup.bResetToFullLife;
	if (Setup.StateTree)
	{
		StateTree = Setup.StateTree;
	}

	if (Setup.BehaviourStateTree)
	{
		BehaviourStateTree = Setup.BehaviourStateTree;
	}

	AbilitySystemComponent->StartupAbilityTags.Append(Setup.AbilityTags);
	if (!Setup.Tag.IsNone())
	{
		Tags.Add(Setup.Tag);
	}

	AggroRadiusOverride = Setup.AggroRadius;
	MaxHealthOverride = Setup.MaxHealth;
	StartHealthRatio = Setup.StartHealthRatio;
}

void AEnemyCharacter::ApplyMaxHealthOverride()
{
	float const MaxHealth = MaxHealthOverride.GetValueAtLevel(AbilitySystemComponent->GetCombatLevel());
	if (MaxHealth > 0.f)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), MaxHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), MaxHealth);
	}
}

void AEnemyCharacter::ResetAttributes()
{
	AbilitySystemComponent->InitializeDefaultAttributes();
	ApplyMaxHealthOverride();
}

void AEnemyCharacter::OnHealthChanged_Implementation(float NewValue)
{
	if (GeoLib::IsServer(this) && NewValue <= 0.f)
	{
		if (ResetToFullLifeWhenReachingZero)
		{
			ResetAttributes();
		}
		else
		{
			OnEnemyDefeated.Broadcast();
			Death();
		}
	}
}

void AEnemyCharacter::DeathLogic()
{
	StopAllSpawnedElements();
	AbilitySystemComponent->CancelAllAbilities();
	DetachFromControllerPendingDestroy();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
	Super::DeathLogic();
}

AEnemyCharacter* AEnemyCharacter::SpawnEnemy(UWorld& World, TSubclassOf<AEnemyCharacter> const EnemyClass,
											 FVector Location, AActor* const Owner, AGeoArena* const Arena,
											 FGeoEnemySetup const* const Setup)
{
	Location.Z = ArbitraryCharacterZ;
	FTransform const Transform(FRotator::ZeroRotator, Location);
	AEnemyCharacter* const Enemy = World.SpawnActorDeferred<AEnemyCharacter>(
		EnemyClass, Transform, Owner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (ensureMsgf(Enemy, TEXT("Failed to spawn %s"), *GetNameSafe(EnemyClass)))
	{
		Enemy->Arena = Arena;
		if (Setup)
		{
			Enemy->ApplySetup(*Setup);
		}

		Enemy->FinishSpawning(Transform);
	}

	return Enemy;
}

bool AEnemyCharacter::IsFighting() const
{
	return Arena.IsValid() && Arena->IsFighting();
}

void AEnemyCharacter::ResetForNewAttempt()
{
	StopAllSpawnedElements();
	ResetAttributes();
	if (AGeoEnemyAIController* AIController = GetController<AGeoEnemyAIController>())
	{
		AIController->ResetAI();
	}
}
