// GeoEnemyAIController.cpp

#include "AI/GeoEnemyAIController.h"

#include "Characters/EnemyCharacter.h"
#include "Components/StateTreeAIComponent.h"

AGeoEnemyAIController::AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComp"));
}

void AGeoEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (AEnemyCharacter const* EnemyChar = Cast<AEnemyCharacter>(InPawn))
	{
		if (EnemyChar->StateTree)
		{
			StateTreeComp->SetStateTree(EnemyChar->StateTree);
			StateTreeComp->StartLogic();
		}
	}
}
