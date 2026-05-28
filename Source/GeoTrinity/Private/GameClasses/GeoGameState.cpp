// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameState.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/GeoArenaBarrier.h"
#include "AI/GeoEnemyAIController.h"
#include "Characters/EnemyCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/TargetPoint.h"
#include "GameClasses/GeoGameMode.h"
#include "HUD/GeoHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (HasAuthority() && EnemiesToSpawn.Num() > 0)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;
		for (auto EnemyToSpawn : EnemiesToSpawn)
		{
			AEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacter>(
				EnemyToSpawn, FTransform(FVector(100, 100, ArbitraryCharacterZ)), SpawnParams);
			SpawnedEnemies.Add(SpawnedEnemy);
			OnEnemySpawned.Broadcast(SpawnedEnemy);
		}
	}

	if (AEnemyCharacter* Boss = GetFirstEnemy())
	{
		if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
		{
			if (AGeoHUD* HUD = LocalPC->GetHUD<AGeoHUD>())
			{
				HUD->ShowBossHealthBar(Boss);
			}
		}

		if (HasAuthority())
		{
			Boss->OnBossDefeated.AddDynamic(this, &AGeoGameState::NotifyBossDefeated);

			if (AGeoEnemyAIController* AIC = Cast<AGeoEnemyAIController>(Boss->GetController()))
			{
				AIC->GetStateTreeComp()->SendStateTreeEvent(FGeoGameplayTags::Get().AI_Boss_Aggro);
			}

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
}

void AGeoGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (AGeoHUD* HUD = LocalPC->GetHUD<AGeoHUD>())
		{
			HUD->HideBossHealthBar();
		}
	}

	if (HasAuthority())
	{
		if (ArenaBarrier)
		{
			ArenaBarrier->SetClosed(false);
		}
		TeleportPlayersTo(FGeoGameplayTags::Get().AI_Arena_Entrance);
	}
}

void AGeoGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (AGeoHUD* HUD = LocalPC->GetHUD<AGeoHUD>())
		{
			HUD->HideBossHealthBar();
		}
	}

	if (HasAuthority() && ArenaBarrier)
	{
		ArenaBarrier->SetClosed(false);
	}
}

void AGeoGameState::CommitFightStart()
{
	if (ArenaBarrier)
	{
		ArenaBarrier->SetClosed(true);
	}
	TeleportPlayersTo(FGeoGameplayTags::Get().AI_Arena_PlayerSpawn);
}

void AGeoGameState::TeleportPlayersTo(FGameplayTag LocationTag)
{
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), ATargetPoint::StaticClass(),
		LocationTag.GetTagName(), SpawnPoints);

	if (SpawnPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GeoGameState::TeleportPlayersTo — no ATargetPoint found for tag %s"), *LocationTag.ToString());
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

void AGeoGameState::NotifyPlayerDiedInFight()
{
	--PlayersAliveInFight;
	if (PlayersAliveInFight > 0)
	{
		return;
	}

	if (AEnemyCharacter* Boss = GetFirstEnemy())
	{
		Boss->ResetForNewAttempt();
	}

	if (AGeoGameMode* GM = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RequestWaitingToStart();
	}
}

void AGeoGameState::NotifyBossDefeated()
{
	if (AGeoGameMode* GM = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RequestWaitingPostMatch();
	}
}
