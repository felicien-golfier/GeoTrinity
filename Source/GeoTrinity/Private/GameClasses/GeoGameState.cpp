// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameState.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/GeoArenaBarrier.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "GameClasses/GeoGameMode.h"
#include "GameplayTagContainer.h"
#include "HUD/GeoHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::HandleMatchHasStarted()
{
	AEnemyCharacter* Boss = GetBossEnemy();
	if (ensureMsgf(IsValid(Boss),
				   TEXT("No Boss found in the world, ensure an EnemyCharacter with bIsBoss=true is spawned")))
	{
		InitBossFight(Boss);
	}
}

void AGeoGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	TeleportPlayersTo(FGeoGameplayTags::Get().Arena_Entrance, EntranceZoneTagName);

	SpawnEnemies();

	MatchIsWaitingToStartDelegate.Broadcast();
}

void AGeoGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	StopBossFight();
}

void AGeoGameState::OnRep_MatchState()
{
	if (PreviousMatchState != MatchState && PreviousMatchState == MatchState::InProgress)
	{
		StopBossFight();
	}
	Super::OnRep_MatchState();
}

void AGeoGameState::SpawnEnemies()
{
	if (!GeoLib::IsServer(this) || (!BossToSpawn.EnemyClass.Get() && !DummyToSpawn.EnemyClass.Get()))
	{
		return;
	}

	if (!GetBossEnemy() || GetBossEnemy()->IsActorBeingDestroyed())
	{
		SpawnEnemy(BossToSpawn);
	}

	if (!GetDummyEnemy() || GetDummyEnemy()->IsActorBeingDestroyed())
	{
		SpawnEnemy(DummyToSpawn);
	}
}

void AGeoGameState::SpawnEnemy(FEnemySpawnEntry const& Entry)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	TArray<AActor*> Points = GeoLib::GetTargetPoints(this, Entry.SpawnTag);
	FVector SpawnLocation = Points.Num() > 0 ? Points[0]->GetActorLocation() : FVector(100.f, 100.f, 0.f);
	SpawnLocation.Z = ArbitraryCharacterZ;
	GetWorld()->SpawnActor<AEnemyCharacter>(Entry.EnemyClass, FTransform(SpawnLocation), SpawnParams);
}

bool AGeoGameState::IsBoss(AActor const* Enemy) const
{
	return IsValid(Enemy) && Enemy->GetClass() == BossToSpawn.EnemyClass;
}

bool AGeoGameState::IsDummy(AActor const* Enemy) const
{
	return IsValid(Enemy) && Enemy->GetClass() == DummyToSpawn.EnemyClass;
}

AEnemyCharacter* AGeoGameState::GetBossEnemy() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AEnemyCharacter::StaticClass(), Enemies);
	for (AActor* Enemy : Enemies)
	{
		if (IsBoss(Enemy))
		{
			return Cast<AEnemyCharacter>(Enemy);
		}
	}
	return nullptr;
}

AEnemyCharacter* AGeoGameState::GetDummyEnemy() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AEnemyCharacter::StaticClass(), Enemies);
	for (AActor* Enemy : Enemies)
	{
		if (IsDummy(Enemy))
		{
			return Cast<AEnemyCharacter>(Enemy);
		}
	}
	return nullptr;
}

void AGeoGameState::StopBossFight()
{
	if (GetBossEnemy())
	{
		GetBossEnemy()->Destroy();
	}

	if (APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (AGeoHUD* GeoHUD = LocalPlayerController->GetHUD<AGeoHUD>())
		{
			GeoHUD->HideBossHealthBar();
		}
	}

	if (GeoLib::IsServer(this))
	{
		GetWorld()->GetTimerManager().ClearTimer(CommitFightTimer);
		if (AGeoArenaBarrier* ArenaBarrier = GetArenaBarrier())
		{
			ArenaBarrier->SetClosed(false);
		}

		RevivePlayers();
	}
}

void AGeoGameState::InitBossFight(AEnemyCharacter* Boss)
{
	if (APlayerController const* LocalPlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (AGeoHUD* GeoHUD = LocalPlayerController->GetHUD<AGeoHUD>())
		{
			GeoHUD->ShowBossHealthBar(Boss);
		}
	}

	if (GeoLib::IsServer(this))
	{
		ensureMsgf(!Boss->OnBossDefeated.IsAlreadyBound(this, &AGeoGameState::NotifyBossDefeated),
				   TEXT("Boss %s already has OnBossDefeated bound to this GameState"), *Boss->GetName());

		Boss->OnBossDefeated.AddUniqueDynamic(this, &AGeoGameState::NotifyBossDefeated);

		if (AGeoEnemyAIController* EnemyAIController = Cast<AGeoEnemyAIController>(Boss->GetController()))
		{
			EnemyAIController->GetStateTreeComp()->SendStateTreeEvent(FGeoGameplayTags::Get().AI_Boss_AggroEvent);
		}

		// TODO : Do a proper player life cycle not boss dependent
		PlayersAliveInFight = 0;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				++PlayersAliveInFight;
			}
		}

		if (AGeoArenaBarrier* ArenaBarrier = GetArenaBarrier())
		{
			ArenaBarrier->SetClosed(true);
		}

		GetWorld()->GetTimerManager().SetTimer(CommitFightTimer, this, &AGeoGameState::CommitFightStart,
											   CommitFightTime, false);
	}
}

AGeoArenaBarrier* AGeoGameState::GetArenaBarrier() const
{
	return Cast<AGeoArenaBarrier>(UGameplayStatics::GetActorOfClass(this, AGeoArenaBarrier::StaticClass()));
}

void AGeoGameState::CommitFightStart() const
{
	TeleportPlayersTo(FGeoGameplayTags::Get().Arena_FightLocation, FightZoneTagName);
	CommitFightDelegate.Broadcast();
}

void AGeoGameState::TeleportPlayersTo(FGameplayTag const LocationTag, FName const& ExemptZoneName) const
{
	TArray<AActor*> SpawnPoints = GeoLib::GetTargetPoints(this, LocationTag);
	if (SpawnPoints.IsEmpty())
	{
		ensureMsgf(false, TEXT("Ensure to add Spawn points with tag %s in your map, DUMBASS"),
				   *LocationTag.GetTagName().ToString());
		return;
	}

	int32 SpawnIndex = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			continue;
		}
		APawn* Pawn = (*It)->GetPawn();
		bool bTeleport = IsValid(Pawn);

		TArray<AActor*> ExemptZone;
		UGameplayStatics::GetAllActorsWithTag(this, ExemptZoneName, ExemptZone);
		for (AActor const* Zone : ExemptZone)
		{
			bTeleport &= !Pawn->IsOverlappingActor(Zone);
		}

		if (bTeleport)
		{
			AActor const* SpawnPoint = SpawnPoints[SpawnIndex % SpawnPoints.Num()];
			Pawn->SetActorLocation(SpawnPoint->GetActorLocation());
			++SpawnIndex;
		}
	}
}

void AGeoGameState::RequestWaitingToStart() const
{
	if (AGeoGameMode* GeoGameMode = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GeoGameMode->RequestWaitingToStart();
	}
}

void AGeoGameState::RevivePlayers() const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayableCharacter* Player = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr)
		{
			Player->Revive();
		}
	}
}

void AGeoGameState::NotifyPlayerDiedInFight(APlayableCharacter* /*PlayableCharacter*/)
{
	--PlayersAliveInFight;
	if (PlayersAliveInFight > 0)
	{
		return;
	}

	if (AEnemyCharacter* Boss = GetBossEnemy())
	{
		Boss->ResetForNewAttempt();
	}

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGeoGameState::RequestWaitingToStart, DeathTime, false);
}

void AGeoGameState::NotifyBossDefeated()
{
	if (AGeoGameMode* GeoGameMode = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GeoGameMode->RequestWaitingPostMatch();
	}
}
