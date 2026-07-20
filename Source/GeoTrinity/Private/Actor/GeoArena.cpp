// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoArena.h"

#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/GeoArenaBarrier.h"
#include "Characters/EnemyCharacter.h"
#include "Components/SceneComponent.h"
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
}

void AGeoArena::CommitFight()
{
}

void AGeoArena::EndFight()
{
	if (Barrier)
	{
		Barrier->SetClosed(false);
	}
}

bool AGeoArena::IsBoss(AActor const* Enemy) const
{
	return IsValid(Enemy) && Enemy == Boss;
}
