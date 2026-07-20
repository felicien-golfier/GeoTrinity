// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoArena.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/GeoArenaBarrier.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/SceneComponent.h"
#include "GameClasses/GeoGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoArena::AGeoArena()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetCanBeDamaged(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	GetRootComponent()->SetMobility(EComponentMobility::Static);
}

void AGeoArena::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoArena, Boss);
}

void AGeoArena::OnRep_Boss()
{
	if (IsValid(Boss))
	{
		Boss->Arena = this;
	}
}

void AGeoArena::BeginPlay()
{
	Super::BeginPlay();
	if (GeoLib::IsServer(this))
	{
		ResetBoss();
	}
}

void AGeoArena::ResetBoss()
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("%s: ResetBoss is server-only"), *GetName())
		|| !ensureMsgf(BossClass, TEXT("%s: no BossClass configured"), *GetName()))
	{
		return;
	}
	if (IsValid(Boss) && !Boss->IsActorBeingDestroyed())
	{
		Boss->ResetForNewAttempt();
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	TArray<AActor*> const SpawnPoints =
		GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_BossSpawn, ArenaTag);
	FVector SpawnLocation = SpawnPoints.IsEmpty() ? GetActorLocation() : SpawnPoints[0]->GetActorLocation();
	SpawnLocation.Z = ArbitraryCharacterZ;
	Boss = GetWorld()->SpawnActor<AEnemyCharacter>(BossClass, FTransform(SpawnLocation), SpawnParams);
	Boss->Arena = this;
}

AGeoArena* AGeoArena::GetArenaOfBoss(AActor const* Boss)
{
	if (ensureMsgf(IsValid(Boss), TEXT("Boss you try to get the arena from is invalid")))
	{
		AEnemyCharacter const* BossEnemy = Cast<AEnemyCharacter>(Boss);
		if (ensureMsgf(IsValid(BossEnemy), TEXT("Boss you try to get Arena from is not a AEnemyCharacter")))
		{
			return BossEnemy->Arena.Get();
		}
	}
	return nullptr;
}

void AGeoArena::StartFight()
{
	if (Barrier)
	{
		Barrier->SetClosed(true);
	}
	GetWorld()->GetTimerManager().SetTimer(CommitFightTimer, this, &AGeoArena::CommitFight,
										   GetWorld()->GetGameStateChecked<AGeoGameState>()->CommitFightTime, false);
}

void AGeoArena::CommitFight()
{
	TeleportPlayersTo(FGeoGameplayTags::Get().TargetPoint_FightLocation, FightZoneTagName);
}

void AGeoArena::EndFight()
{
	GetWorld()->GetTimerManager().ClearTimer(CommitFightTimer);
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	if (Barrier)
	{
		Barrier->SetClosed(false);
	}
}

void AGeoArena::RespawnPlayer(APlayableCharacter& /*Player*/)
{
	if (!AreAllPlayersDead())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AGeoArena::RespawnGroup, DeathTime, false);
}

bool AGeoArena::AreAllPlayersDead() const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayableCharacter const* Player = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr;
		if (IsValid(Player) && !Player->IsDead())
		{
			return false;
		}
	}
	return true;
}

void AGeoArena::RespawnGroup()
{
	AGeoGameState const* GameState = GetWorld()->GetGameStateChecked<AGeoGameState>();
	TeleportPlayersTo(FGeoGameplayTags::Get().TargetPoint_Entrance, EntranceZoneTagName);
	GameState->RevivePlayers();
	GameState->RequestWaitingToStart();
}

void AGeoArena::TeleportPlayersTo(FGameplayTag const PurposeTag, FName const ExemptZoneName) const
{
	TArray<AActor*> const SpawnPoints = GeoLib::GetTargetPoints(this, PurposeTag, ArenaTag);
	if (!ensureMsgf(!SpawnPoints.IsEmpty(), TEXT("Ensure to add Spawn points tagged %s + %s in your map, DUMBASS"),
					*PurposeTag.GetTagName().ToString(), *ArenaTag.GetTagName().ToString()))
	{
		return;
	}

	TArray<AActor*> ExemptZone;
	UGameplayStatics::GetAllActorsWithTag(this, ExemptZoneName, ExemptZone);

	int32 SpawnIndex = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->IsValid() ? (*It)->GetPawn() : nullptr;
		if (!IsValid(Pawn))
		{
			continue;
		}

		bool bTeleport = true;
		for (AActor const* Zone : ExemptZone)
		{
			bTeleport &= !Pawn->IsOverlappingActor(Zone);
		}

		if (bTeleport)
		{
			Pawn->SetActorLocation(SpawnPoints[SpawnIndex % SpawnPoints.Num()]->GetActorLocation());
			++SpawnIndex;
		}
	}
}

bool AGeoArena::IsBoss(AActor const* Enemy) const
{
	return IsValid(Enemy) && Enemy == Boss;
}
