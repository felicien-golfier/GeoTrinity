// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/GeoEnemyAIController.h"

#include "AI/GeoAIBlackboardComponent.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Arena/GeoArena.h"
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

	// AGeoCharacter::Tick drives ControlRotation itself (clamped turn toward TargetYaw). Leaving this on would have
	// AAIController::Tick snap ControlRotation back to the pawn's (stale, previous-frame) actor rotation every
	// frame, fighting that clamp and preventing the boss from ever visibly turning.
	bSetControlRotationFromPawnOrientation = false;
}

IGenericTeamAgentInterface* AGeoEnemyAIController::GetPawnTeamAgent() const
{
	IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(GetPawn());
	ensureMsgf(TeamAgentInterface, TEXT("%hs: %s has %s"), __FUNCTION__, *GetName(),
			   GetPawn() ? TEXT("a pawn with no IGenericTeamAgentInterface") : TEXT("no pawn"));
	return TeamAgentInterface;
}

void AGeoEnemyAIController::SetGenericTeamId(FGenericTeamId const& NewTeamId)
{
	if (IGenericTeamAgentInterface* TeamAgentInterface = GetPawnTeamAgent())
	{
		TeamAgentInterface->SetGenericTeamId(NewTeamId);
	}
}

FGenericTeamId AGeoEnemyAIController::GetGenericTeamId() const
{
	IGenericTeamAgentInterface const* TeamAgentInterface = GetPawnTeamAgent();
	return TeamAgentInterface ? TeamAgentInterface->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

void AGeoEnemyAIController::ResetAI()
{
	StopAggroWatch();
	bAggroed = false;
	InitializeForPawn(GetPawn());
}

void AGeoEnemyAIController::InitializeForPawn(APawn* InPawn)
{
	AEnemyCharacter* EnemyChar = Cast<AEnemyCharacter>(InPawn);
	if (!ensureMsgf(IsValid(EnemyChar), TEXT("%hs: pawn is not an AEnemyCharacter"), __FUNCTION__))
	{
		return;
	}

	InitializeStateTree(EnemyChar);
	InitializeAggro(EnemyChar);
}

void AGeoEnemyAIController::InitializeAggro(AEnemyCharacter const* EnemyChar)
{
	StopAggroWatch();

	GetWorld()->GetTimerManager().SetTimer(AggroCheckTimer, this, &AGeoEnemyAIController::CheckAggroDistance, 0.5f,
										   true);

	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(EnemyChar->GetAbilitySystemComponent());
	if (!ensureMsgf(ASC, TEXT("%hs: %s has no GeoAbilitySystemComponent"), __FUNCTION__, *EnemyChar->GetName()))
	{
		return;
	}

	ASC->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &AGeoEnemyAIController::OnGEApplied);
}

void AGeoEnemyAIController::StopAggroWatch()
{
	GetWorld()->GetTimerManager().ClearTimer(AggroCheckTimer);

	UGeoAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(GetPawn());
	if (!ensureMsgf(ASC, TEXT("%hs: %s has no GeoAbilitySystemComponent"), __FUNCTION__, *GetNameSafe(GetPawn())))
	{
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
	InitializeForPawn(InPawn);
}

void AGeoEnemyAIController::OnUnPossess()
{
	StopAggroWatch();
	Super::OnUnPossess();
}

void AGeoEnemyAIController::Tick(float const DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCurrentTarget(DeltaTime);
}

void AGeoEnemyAIController::UpdateCurrentTarget(float const DeltaTime)
{
	APawn const* EnemyPawn = GetPawn();
	if (!EnemyPawn)
	{
		CurrentTarget = nullptr;
		PendingTarget = nullptr;
		PendingTargetElapsedTime = 0.f;
		return;
	}

	FVector2D const PawnLocation(EnemyPawn->GetActorLocation());
	APlayableCharacter* Closest = nullptr;
	float BestDistanceSquared = 0.f;
	for (APlayableCharacter* Player : GeoASLib::GetInteractableActors<APlayableCharacter>(
			 EnemyPawn, GeoASLib::GetTeamId(EnemyPawn), TeamAttitudeMask::Hostile, /*bMustBeDamageable*/ false,
			 PawnLocation,
			 /*MaxDistance*/ 0.f))
	{
		if (Player->IsDead())
		{
			continue;
		}
		float const DistanceSquared = FVector2D::DistSquared(FVector2D(Player->GetActorLocation()), PawnLocation);
		if (!Closest || DistanceSquared < BestDistanceSquared)
		{
			Closest = Player;
			BestDistanceSquared = DistanceSquared;
		}
	}

	if (!Closest || Closest == CurrentTarget)
	{
		CurrentTarget = Closest;
		PendingTarget = nullptr;
		PendingTargetElapsedTime = 0.f;
		return;
	}

	if (Closest != PendingTarget)
	{
		PendingTarget = Closest;
		PendingTargetElapsedTime = 0.f;
	}

	PendingTargetElapsedTime += DeltaTime;
	if (!CurrentTarget || PendingTargetElapsedTime >= TargetSwitchDelay)
	{
		CurrentTarget = PendingTarget;
		PendingTarget = nullptr;
		PendingTargetElapsedTime = 0.f;
	}
}

void AGeoEnemyAIController::CheckAggroDistance()
{
	if (bAggroed || !GetPawn() || GetWorld()->GetGameStateChecked<AGeoGameState>()->IsMatchInProgress())
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
	StopAggroWatch();

	StateTreeComp->SendStateTreeEvent(FGeoGameplayTags::Get().AI_Boss_AggroEvent);

	AGeoArena* Arena = AGeoArena::GetArenaOfBoss(GetPawn());
	AGeoGameMode* GeoGameMode = GetWorld()->GetAuthGameMode<AGeoGameMode>();
	if (Arena && GeoGameMode && Arena->IsBoss(GetPawn()))
	{
		Arena->StartFight();
		GeoGameMode->StartMatch();
	}
}
