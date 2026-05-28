// GeoEnemyAIController.cpp

#include "AI/GeoEnemyAIController.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AI/GeoAIBlackboardComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "GameFramework/GameMode.h"
#include "EngineUtils.h"

AGeoEnemyAIController::AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComp"));
	GeoBlackBoard = CreateDefaultSubobject<UGeoAIBlackboardComponent>(TEXT("GeoBlackBoard"));
}

void AGeoEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(InPawn);
	if (EnemyChar && EnemyChar->StateTree)
	{
		StateTreeComp->SetStateTree(EnemyChar->StateTree);
		StateTreeComp->StartLogic();
	}

	if (HasAuthority() && EnemyChar)
	{
		GetWorld()->GetTimerManager().SetTimer(AggroCheckTimer, this,
			&AGeoEnemyAIController::CheckAggroDistance, 0.5f, true);

		UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(
			EnemyChar->GetAbilitySystemComponent());
		if (ensureMsgf(ASC, TEXT("GeoEnemyAIController::OnPossess — boss has no GeoAbilitySystemComponent")))
		{
			ASC->OnDamageDealt.AddDynamic(this, &AGeoEnemyAIController::OnBossDamaged);
		}
	}
}

void AGeoEnemyAIController::CheckAggroDistance()
{
	if (bAggroed || !GetPawn())
	{
		return;
	}
	FVector2D const BossPos(GetPawn()->GetActorLocation());
	for (APlayableCharacter* PC : TActorRange<APlayableCharacter>(GetWorld()))
	{
		if (FVector2D::Distance(BossPos, FVector2D(PC->GetActorLocation())) <= AggroRadius)
		{
			TriggerAggro();
			return;
		}
	}
}

void AGeoEnemyAIController::OnBossDamaged(float /*DamageAmount*/, FGameplayTag /*AbilityTag*/)
{
	TriggerAggro();
}

void AGeoEnemyAIController::TriggerAggro()
{
	if (bAggroed)
	{
		return;
	}
	bAggroed = true;
	GetWorld()->GetTimerManager().ClearTimer(AggroCheckTimer);

	if (AGameMode* GM = Cast<AGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->StartMatch();
	}
}

void AGeoEnemyAIController::RestartStateTree()
{
	bAggroed = false;
	StateTreeComp->StopLogic(TEXT("Reset"));
	StateTreeComp->StartLogic();
	GetWorld()->GetTimerManager().SetTimer(AggroCheckTimer, this,
		&AGeoEnemyAIController::CheckAggroDistance, 0.5f, true);
}
