// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/GeoEnemyAIController.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "EngineUtils.h"
#include "GameClasses/GeoGameMode.h"
#include "GameClasses/GeoGameState.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoEnemyAIController::AGeoEnemyAIController(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComp"));
	GeoBlackBoard = CreateDefaultSubobject<UGeoAIBlackboardComponent>(TEXT("GeoBlackBoard"));
}

void AGeoEnemyAIController::SetGenericTeamId(FGenericTeamId const& NewTeamId)
{
	IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(GetPawn());
	if (!TeamAgentInterface)
	{
		ensureMsgf(GetPawn(), TEXT("No Pawn on %s"), *GetName());
		ensureMsgf(TeamAgentInterface, TEXT("No IGenericTeamAgentInterface on %s"), *GetName());
		return;
	}

	TeamAgentInterface->SetGenericTeamId(NewTeamId);
}

FGenericTeamId AGeoEnemyAIController::GetGenericTeamId() const
{
	IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(GetPawn());
	if (!TeamAgentInterface)
	{
		ensureMsgf(GetPawn(), TEXT("No Pawn on %s"), *GetName());
		ensureMsgf(TeamAgentInterface, TEXT("No IGenericTeamAgentInterface on %s"), *GetName());
		return FGenericTeamId::NoTeam;
	}

	return TeamAgentInterface->GetGenericTeamId();
}

void AGeoEnemyAIController::ResetAI()
{
	ClearAggro();
	bAggroed = false;
	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(GetPawn());
	if (!IsValid(EnemyChar))
	{
		ensureMsgf(IsValid(EnemyChar), TEXT("Pawn is not an EnemyCharacter"));
		return;
	}

	InitializeStateTree(EnemyChar);
	InitializeAggro(EnemyChar);
}

void AGeoEnemyAIController::InitializeAggro(AEnemyCharacter const* EnemyChar)
{
	ClearAggro();

	GetWorld()->GetTimerManager().SetTimer(AggroCheckTimer, this, &AGeoEnemyAIController::CheckAggroDistance, 0.5f,
										   true);

	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(EnemyChar->GetAbilitySystemComponent());
	if (!ASC)
	{
		ensureMsgf(false, TEXT("GeoEnemyAIController::OnPossess — boss has no GeoAbilitySystemComponent"));
		return;
	}

	ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &AGeoEnemyAIController::OnGEApplied);
}

void AGeoEnemyAIController::ClearAggro()
{
	GetWorld()->GetTimerManager().ClearTimer(AggroCheckTimer);

	UGeoAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(GetPawn());
	if (!ASC)
	{
		ensureMsgf(false, TEXT("GeoEnemyAIController::OnPossess — boss has no GeoAbilitySystemComponent"));
		return;
	}

	ASC->OnGameplayEffectAppliedDelegateToSelf.RemoveAll(this);
}

void AGeoEnemyAIController::InitializeStateTree(AEnemyCharacter const* EnemyChar) const
{
	if (EnemyChar->StateTree)
	{
		StateTreeComp->SetStateTree(EnemyChar->StateTree);
		StateTreeComp->StartLogic();
	}
}

void AGeoEnemyAIController::OnPossess(APawn* InPawn)
{
	// Only called on server
	Super::OnPossess(InPawn);
	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(InPawn);
	if (!IsValid(EnemyChar))
	{
		ensureMsgf(IsValid(EnemyChar), TEXT("Pawn is not an EnemyCharacter"));
		return;
	}

	InitializeStateTree(EnemyChar);
	InitializeAggro(EnemyChar);
}

void AGeoEnemyAIController::OnUnPossess()
{
	ClearAggro();
	Super::OnUnPossess();
}

void AGeoEnemyAIController::CheckAggroDistance()
{
	if (bAggroed || !GetPawn())
	{
		return;
	}
	FVector2D const BossPos(GetPawn()->GetActorLocation());
	TArray<AActor*> Actors = GeoASLib::GetInteractableActors(this, GetGenericTeamId(), TeamAttitudeMask::Hostile, false,
															 BossPos, AggroRadius);
	if (Actors.Num() > 0)
	{
		TriggerAggro();
	}
}

void AGeoEnemyAIController::OnGEApplied(UAbilitySystemComponent* Source, FGameplayEffectSpec const&,
										FActiveGameplayEffectHandle)
{
	if (Source != GeoASLib::GetGeoAscFromActor(GetPawn()))
	{
		TriggerAggro();
	}
}

void AGeoEnemyAIController::TriggerAggro()
{
	if (bAggroed)
	{
		return;
	}
	bAggroed = true;
	ClearAggro();

	AGeoGameState const* GeoGameState = GetWorld()->GetGameStateChecked<AGeoGameState>();
	AGeoGameMode* GeoGameMode = GetWorld()->GetAuthGameMode<AGeoGameMode>();
	if (GeoGameState && GeoGameMode && GeoGameState->IsBoss(GetPawn()))
	{
		GeoGameMode->StartMatch();
	}
}
