// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/EnemyCharacter.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/InteractableComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(
		  ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Ensure an AI controller and auto possession for enemies
	AIControllerClass = AGeoEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		// Mark as Enemy team and bind health change to handle death
		InteractableComponent->SetGenericTeamId(FGenericTeamId(static_cast<uint8>(ETeam::Enemy)));
		InteractableComponent->OnHealthChanged.AddDynamic(this, &AEnemyCharacter::OnHealthChanged);
	}

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Path"), FiringPoints);
}

void AEnemyCharacter::OnHealthChanged(float NewValue)
{
	if (NewValue <= 0.f)
	{
		Destroy();
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