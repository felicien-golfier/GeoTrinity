// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameState.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Deployable/BuffPickup/GeoBuffPickup.h"
#include "Actor/GeoArenaBarrier.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "GameClasses/GeoGameMode.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"
#include "HUD/Interface/GeoHUDInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::HandleMatchHasStarted()
{
	AEnemyCharacter* Boss = GetBossEnemy();
	if (!GeoLib::IsServer(this) && !IsValid(Boss))
	{
		// Late-joining client: MatchState replicated before the Boss actor did. It will show up shortly;
		// nothing to do here since the server already drove StartBossFight().
		UE_LOG(LogTemp, Log, TEXT("HandleMatchHasStarted: Boss not yet replicated to this client"));
		return;
	}

	if (ensureMsgf(IsValid(Boss),
				   TEXT("No Boss found in the world, ensure an EnemyCharacter with bIsBoss=true is spawned")))
	{
		StartBossFight(Boss);
	}
}

void AGeoGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	GetWorld()->GetTimerManager().ClearTimer(LootTimer);
	for (TWeakObjectPtr<UGeoDeployableManagerComponent> const& Manager : LootBoostedManagers)
	{
		if (UGeoDeployableManagerComponent* DeployableManager = Manager.Get())
		{
			DeployableManager->RemoveDeployableSlot(LootPickupClass);
		}
	}
	LootBoostedManagers.Empty();

	TeleportPlayersTo(FGeoGameplayTags::Get().Arena_Entrance, EntranceZoneTagName);

	ResetEnemies();
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
	OnMatchStateChanged.Broadcast(MatchState, PreviousMatchState);
	Super::OnRep_MatchState();
}

void AGeoGameState::ResetEnemies()
{
	if (!GeoLib::IsServer(this) || (!BossToSpawn.EnemyClass.Get() && !DummyToSpawn.EnemyClass.Get()))
	{
		return;
	}

	if (!GetBossEnemy() || GetBossEnemy()->IsActorBeingDestroyed())
	{
		SpawnEnemy(BossToSpawn);
	}
	else
	{
		GetBossEnemy()->ResetForNewAttempt();
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
		if (IGeoHUDInterface* GeoHUD = Cast<IGeoHUDInterface>(LocalPlayerController->GetHUD()))
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

void AGeoGameState::StartBossFight(AEnemyCharacter* Boss)
{
	if (APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (IGeoHUDInterface* GeoHUD = Cast<IGeoHUDInterface>(LocalPlayerController->GetHUD()))
		{
			GeoHUD->ShowBossHealthBar(Boss);
		}
	}

	if (GeoLib::IsServer(this))
	{
		ensureMsgf(!Boss->OnEnemyDefeated.IsAlreadyBound(this, &AGeoGameState::NotifyBossDefeated),
				   TEXT("Boss %s already has OnEnemyDefeated bound to this GameState"), *Boss->GetName());

		Boss->OnEnemyDefeated.AddUniqueDynamic(this, &AGeoGameState::NotifyBossDefeated);

		if (AGeoEnemyAIController* EnemyAIController = Cast<AGeoEnemyAIController>(Boss->GetController()))
		{
			EnemyAIController->GetStateTreeComp()->SendStateTreeEvent(FGeoGameplayTags::Get().AI_Boss_AggroEvent);
		}

		PlayersInFight.Reset();
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayableCharacter* Player = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr)
			{
				PlayersInFight.Add(Player);
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

bool AGeoGameState::AreAllPlayersDead() const
{
	for (APlayableCharacter* Player : PlayersInFight)
	{
		if (IsValid(Player) && !Player->IsDead())
		{
			return false;
		}
	}
	return true;
}

void AGeoGameState::NotifyPlayerDied(APlayableCharacter* PlayableCharacter)
{
	if (MatchState != MatchState::InProgress)
	{
		// Player died while not in progress (e.g. during the fight-commit transition); just revive that player.
		PlayableCharacter->Revive();
		return;
	}

	HandlePotentialWipe();
}

void AGeoGameState::NotifyPlayerLeft(APlayableCharacter* PlayableCharacter)
{
	PlayersInFight.Remove(PlayableCharacter);
	HandlePotentialWipe();
}

void AGeoGameState::HandlePotentialWipe()
{
	if (!AreAllPlayersDead())
	{
		return;
	}

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AGeoGameState::RequestWaitingToStart, DeathTime, false);
}

void AGeoGameState::NotifyBossDefeated()
{
	// Capture before the WaitingPostMatch transition: HandleMatchHasEnded → StopBossFight destroys the boss.
	if (AEnemyCharacter const* Boss = GetBossEnemy())
	{
		LootOrigin = Boss->GetActorLocation();
	}

	if (AGeoGameMode* GeoGameMode = Cast<AGeoGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GeoGameMode->RequestWaitingPostMatch();
	}
	Loot();
}

void AGeoGameState::Loot()
{
	if (!GeoLib::IsServer(this))
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(LootTimer, this, &AGeoGameState::SpawnLootBurst, LootSpawnInterval, true);
}

void AGeoGameState::SpawnLootBurst()
{
	// Resolve the Blueprint-derived reload ability CDO that owns the pickup config (class, buff pool, color palette).
	// The ability catalog is keyed by the Spell AbilityTag, which has no native constant, so find the entry by class.
	UGeoReloadAbility const* ReloadCDO = nullptr;
	FGameplayTag ReloadTag;
	if (UAbilityInfo const* AbilityInfo = GeoASLib::GetAbilityInfo())
	{
		for (FGameplayAbilityInfo const& Info : AbilityInfo->GetAllAbilityInfos())
		{
			if (Info.AbilityClass && Info.AbilityClass->IsChildOf(UGeoReloadAbility::StaticClass()))
			{
				ReloadCDO = Info.AbilityClass->GetDefaultObject<UGeoReloadAbility>();
				ReloadTag = Info.AbilityTag;
				break;
			}
		}
	}
	if (!ensureMsgf(ReloadCDO && ReloadCDO->BuffPickupClass,
					TEXT("SpawnLootBurst: no reload ability with a BuffPickupClass registered in AbilityInfo")))
	{
		GetWorld()->GetTimerManager().ClearTimer(LootTimer);
		return;
	}

	TArray<TInstancedStruct<FEffectData>> const BuffEffects = ReloadCDO->GetEffectDataArray();

	// The pickup needs a live player as Owner: its ASC is the effect source and drives the Friendly attitude check.
	APlayableCharacter* PayloadOwner = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		PayloadOwner = It->IsValid() ? Cast<APlayableCharacter>((*It)->GetPawn()) : nullptr;
		if (IsValid(PayloadOwner))
		{
			break;
		}
	}
	if (BuffEffects.IsEmpty() || !IsValid(PayloadOwner))
	{
		return;
	}

	// The pickups register on PayloadOwner's deployable manager; lift its cap for the shower so pickups
	// don't expire each other. Restored in HandleMatchIsWaitingToStart.
	if (UGeoDeployableManagerComponent* DeployableManager =
			PayloadOwner->GetComponentByClass<UGeoDeployableManagerComponent>())
	{
		DeployableManager->SetDeployableInfinitCount(ReloadCDO->BuffPickupClass);
		LootBoostedManagers.AddUnique(DeployableManager);
		LootPickupClass = ReloadCDO->BuffPickupClass;
	}

	FAbilityPayload Payload;
	Payload.Origin = FVector2D(LootOrigin);
	Payload.ServerSpawnTime = GetWorld()->GetTimeSeconds();
	Payload.AbilityTag = ReloadTag;
	Payload.Owner = PayloadOwner;
	Payload.Instigator = PayloadOwner;

	FTransform const SpawnTransform{LootOrigin};
	for (int32 PickupIndex = 0; PickupIndex < LootPickupsPerBurst; ++PickupIndex)
	{
		AGeoBuffPickup* Pickup = GetWorld()->SpawnActorDeferred<AGeoBuffPickup>(
			ReloadCDO->BuffPickupClass, SpawnTransform, PayloadOwner, PayloadOwner,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!ensureMsgf(IsValid(Pickup), TEXT("SpawnLootBurst: failed to spawn AGeoBuffPickup")))
		{
			return;
		}
		// Server-only spawning of replicated actors — no client prediction, so plain RNG is fine here.
		float const Angle = FMath::FRandRange(0.f, 2.f * PI);
		float const Radius = LootMaxRadius * FMath::Sqrt(FMath::FRand()); // sqrt → uniform over the disc
		float const PowerScale = FMath::FRandRange(0.3f, 1.f);
		Payload.Seed = FMath::Rand();
		int32 const BuffIndex = Payload.Seed % BuffEffects.Num();

		FBuffPickupData PickupData;
		GeoASLib::FillDeployableData(PickupData, Payload, BuffEffects, FDeployableDataParams());
		PickupData.EffectDataArray = {BuffEffects[BuffIndex]};
		PickupData.BuffIndex = BuffIndex;
		PickupData.PowerScale = PowerScale;
		PickupData.Level = FMath::RoundToInt32(PowerScale * 10.f);
		PickupData.TargetLocation = LootOrigin + FVector{FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f};

		Pickup->InitInteractable(&PickupData);
		Pickup->FinishSpawning(SpawnTransform);
	}
}
