// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/EnemyCharacter.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(
		  ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(ACharacter::CharacterMovementComponentName))
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

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Path"), FiringPoints);
	InitGAS();
}

void AEnemyCharacter::InitGAS()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Super::InitGAS();

	AbilitySystemComponent->OnHealthChanged.AddDynamic(this,
		&AEnemyCharacter::OnHealthChanged);   // Do we need to remove this on destroy?
}

void AEnemyCharacter::OnHealthChanged(float NewValue)
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
			const UGeoAttributeSetBase* AS = Cast<UGeoAttributeSetBase>(
				AbilitySystemComponent->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
			ModifierInfo.ModifierMagnitude = FScalableFloat(AS->GetMaxHealth());

			GE->Modifiers.Add(ModifierInfo);

			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(AbilitySystemComponent->GetAvatarActor());

			const FGameplayEffectSpec Spec(GE, Context, 1.f);

			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(Spec);
		}
		else
		{
			Destroy();
		}
	}
}

bool AEnemyCharacter::GetAndAdvanceNextFiringPointLocation(FVector& OutLocation)
{
	const int32 Num = FiringPoints.Num();
	if (Num <= 0)
	{
		return false;
	}

	// Wrap index and fetch
	if (CurrentFiringPointIndex >= Num)
	{
		CurrentFiringPointIndex = 0;
	}
	// Find a valid actor starting from current index, loop at most Num times
	int32 Checked = 0;
	while (Checked < Num)
	{
		const int32 Index = (CurrentFiringPointIndex + Checked) % Num;
		if (IsValid(FiringPoints[Index]))
		{
			OutLocation = FiringPoints[Index]->GetActorLocation();
			CurrentFiringPointIndex = (Index + 1) % Num;
			return true;
		}
		++Checked;
	}
	return false;
}