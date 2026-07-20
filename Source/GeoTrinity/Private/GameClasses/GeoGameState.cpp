// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GameClasses/GeoGameState.h"

#include "AI/GeoEnemyAIController.h"
#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Deployable/BuffPickup/GeoBuffPickup.h"
#include "Actor/GeoArena.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "GameClasses/GeoGameMode.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"
#include "HUD/Interface/GeoHUDInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

void AGeoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoGameState, ActiveArena);
}

void AGeoGameState::HandleMatchHasStarted()
{
	AEnemyCharacter* Boss = ActiveArena ? ActiveArena->GetBoss() : nullptr;
	if (!GeoLib::IsServer(this) && !IsValid(Boss))
	{
		// Late-joining client: MatchState replicated before the arena or the Boss actor did. They will show up
		// shortly; nothing to do here since the server already drove StartBossFight().
		UE_LOG(LogTemp, Log, TEXT("HandleMatchHasStarted: Boss not yet replicated to this client"));
		return;
	}

	if (ensureMsgf(IsValid(Boss), TEXT("Match started with no boss on the active arena")))
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

	if (GeoLib::IsServer(this))
	{
		if (ActiveArena)
		{
			ActiveArena->ResetBoss();
		}
		else
		{
			// The level's first WaitingToStart runs before any actor BeginPlay, so no arena can claim this itself.
			SetActiveArena(FindArena(DefaultArenaTag));
		}
	}

	TeleportPlayersTo(FGeoGameplayTags::Get().TargetPoint_Entrance, EntranceZoneTagName);
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

void AGeoGameState::StopBossFight()
{
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
		if (ActiveArena)
		{
			ActiveArena->EndFight();
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

		ActiveArena->StartFight();

		GetWorld()->GetTimerManager().SetTimer(CommitFightTimer, this, &AGeoGameState::CommitFightStart,
											   CommitFightTime, false);
	}
}

void AGeoGameState::CommitFightStart()
{
	if (!ensureMsgf(ActiveArena, TEXT("Fight-commit timer fired with no active arena")))
	{
		return;
	}
	TeleportPlayersTo(FGeoGameplayTags::Get().TargetPoint_FightLocation, FightZoneTagName);
	ActiveArena->CommitFight();
}

FGameplayTag AGeoGameState::GetActiveArenaTag() const
{
	return ActiveArena ? ActiveArena->ArenaTag : DefaultArenaTag;
}

void AGeoGameState::SetActiveArena(AGeoArena* Arena)
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("SetActiveArena is server-only"))
		|| !ensureMsgf(Arena, TEXT("SetActiveArena called with no arena — check the caller's Arena.* tag"))
		|| ActiveArena == Arena)
	{
		return;
	}
	ActiveArena = Arena;
	OnRep_ActiveArena();
}

void AGeoGameState::OnRep_ActiveArena()
{
	OnActiveArenaChanged.Broadcast();
}

AGeoArena* AGeoGameState::FindArena(FGameplayTag const ArenaTag) const
{
	TArray<AActor*> Arenas;
	UGameplayStatics::GetAllActorsOfClass(this, AGeoArena::StaticClass(), Arenas);
	for (AActor* Actor : Arenas)
	{
		AGeoArena* Arena = CastChecked<AGeoArena>(Actor);
		if (Arena->ArenaTag == ArenaTag)
		{
			return Arena;
		}
	}
	return nullptr;
}

void AGeoGameState::TeleportPlayersTo(FGameplayTag const PurposeTag, FName const& ExemptZoneName) const
{
	FGameplayTag const ArenaTag = GetActiveArenaTag();
	TArray<AActor*> SpawnPoints = GeoLib::GetTargetPoints(this, PurposeTag, ArenaTag);
	if (SpawnPoints.IsEmpty())
	{
		ensureMsgf(false, TEXT("Ensure to add Spawn points tagged %s + %s in your map, DUMBASS"),
				   *PurposeTag.GetTagName().ToString(), *ArenaTag.GetTagName().ToString());
		return;
	}

	TArray<AActor*> ExemptZone;
	UGameplayStatics::GetAllActorsWithTag(this, ExemptZoneName, ExemptZone);

	int32 SpawnIndex = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			continue;
		}
		APawn* Pawn = (*It)->GetPawn();
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
	// Capture before the boss destroys itself right after broadcasting its defeat.
	if (ActiveArena && IsValid(ActiveArena->GetBoss()))
	{
		LootOrigin = ActiveArena->GetBoss()->GetActorLocation();
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
