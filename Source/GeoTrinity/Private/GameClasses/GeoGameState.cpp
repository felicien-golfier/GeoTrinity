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
	if (IsValid(Boss))
	{
		InitBoss(Boss);
	}
	else
	{
		ensureMsgf(false, TEXT("No Boss found in the world, ensure an EnemyCharacter with bIsBoss=true is spawned"));
	}
}

void AGeoGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (PreviousMatchState == MatchState::EnteringMap)
	{
		SpawnEnemies();
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
		if (ArenaBarrier)
		{
			ArenaBarrier->SetClosed(false);
		}
		TeleportPlayersTo(FGeoGameplayTags::Get().Arena_Entrance);
	}

	MatchIsWaitingToStartDelegate.Broadcast();
}

void AGeoGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	if (APlayerController const* LocalPlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (AGeoHUD* GeoHUD = LocalPlayerController->GetHUD<AGeoHUD>())
		{
			GeoHUD->HideBossHealthBar();
		}
	}

	if (GeoLib::IsServer(this) && ArenaBarrier)
	{
		ArenaBarrier->SetClosed(false);
	}
}

void AGeoGameState::SpawnEnemies()
{
	if (!GeoLib::IsServer(this) || (!BossToSpawn.EnemyClass.Get() && !DummyToSpawn.EnemyClass.Get()))
	{
		return;
	}

	SpawnEnemy(BossToSpawn, true);
	SpawnEnemy(DummyToSpawn, false);
}

void AGeoGameState::SpawnEnemy(FEnemySpawnEntry const& Entry, bool const bIsBoss)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	TArray<AActor*> Points = GeoLib::GetTargetPoints(this, Entry.SpawnTag);
	FVector SpawnLocation = Points.Num() > 0 ? Points[0]->GetActorLocation() : FVector(100.f, 100.f, 0.f);
	SpawnLocation.Z = ArbitraryCharacterZ;
	AEnemyCharacter* SpawnedEnemy =
		GetWorld()->SpawnActor<AEnemyCharacter>(Entry.EnemyClass, FTransform(SpawnLocation), SpawnParams);
	OnEnemySpawned.Broadcast(SpawnedEnemy);
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

void AGeoGameState::InitBoss(AEnemyCharacter* Boss)
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

		GetWorld()->GetTimerManager().SetTimer(CommitFightTimer, this, &AGeoGameState::CommitFightStart, 5.f, false);
	}
}

void AGeoGameState::CommitFightStart()
{
	if (ArenaBarrier)
	{
		ArenaBarrier->SetClosed(true);
	}
	TeleportPlayersTo(FGeoGameplayTags::Get().Arena_FightLocation);
	CommitFightDelegate.Broadcast();
}

void AGeoGameState::TeleportPlayersTo(FGameplayTag LocationTag) const
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
		if (Pawn)
		{
			AActor* SpawnPoint = SpawnPoints[SpawnIndex % SpawnPoints.Num()];
			Pawn->SetActorLocation(SpawnPoint->GetActorLocation());
			++SpawnIndex;
		}
	}
}

void AGeoGameState::NotifyPlayerDiedInFight(APlayableCharacter* PlayableCharacter)
{

	TArray<AActor*> EntrancePoints = GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().Arena_Entrance);
	if (!EntrancePoints.IsEmpty())
	{
		PlayableCharacter->SetActorLocation(
			EntrancePoints[FMath::RandRange(0, EntrancePoints.Num() - 1)]->GetActorLocation());
	}
	else
	{
		ensureMsgf(false, TEXT("APlayableCharacter::NotifyPlayerDiedInFight — no entrance ATargetPoint found"));
	}

	--PlayersAliveInFight;
	if (PlayersAliveInFight > 0)
	{
		return;
	}

	if (AEnemyCharacter* Boss = GetBossEnemy())
	{
		Boss->ResetForNewAttempt();
	}

	if (AGeoGameMode* GeoGameMode = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GeoGameMode->RequestWaitingToStart();
	}
}

void AGeoGameState::NotifyBossDefeated()
{
	if (AGeoGameMode* GeoGameMode = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GeoGameMode->RequestWaitingPostMatch();
	}
}
