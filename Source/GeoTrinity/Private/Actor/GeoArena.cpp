// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/GeoArena.h"

#include "AbilitySystem/Abilities/Triangle/GeoReloadAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Deployable/BuffPickup/GeoBuffPickup.h"
#include "Actor/GeoArenaBarrier.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HUD/Interface/GeoHUDInterface.h"
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
	DOREPLIFETIME(AGeoArena, bFighting);
}

void AGeoArena::OnRep_Boss()
{
	if (IsValid(Boss))
	{
		Boss->Arena = this;
	}
	ApplyBossBar();
}

void AGeoArena::BeginPlay()
{
	Super::BeginPlay();
	if (GeoLib::IsServer(this))
	{
		ResetBoss();
		AGeoGameState* GameState = GetWorld()->GetGameStateChecked<AGeoGameState>();
		GameState->OnMatchStateChanged.AddUniqueDynamic(this, &AGeoArena::OnMatchStateChanged);
		GameState->OnWipe.AddUniqueDynamic(this, &AGeoArena::OnWipe);
		GameState->OnDifficultyChanged.AddUniqueDynamic(this, &AGeoArena::RespawnBoss);
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
		Boss->Destroy();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	TArray<AActor*> const SpawnPoints =
		GeoLib::GetTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_BossSpawn, ArenaTag);
	FVector SpawnLocation = SpawnPoints.IsEmpty() ? GetActorLocation() : SpawnPoints[0]->GetActorLocation();
	SpawnLocation.Z = ArbitraryCharacterZ;
	Boss = GetWorld()->SpawnActor<AEnemyCharacter>(BossClass, FTransform(SpawnLocation), SpawnParams);
	if (!ensureMsgf(Boss, TEXT("%s: failed to spawn %s"), *GetName(), *BossClass->GetName()))
	{
		return;
	}
	Boss->Arena = this;
}

void AGeoArena::SetBarrierClosed(bool const bClosed) const
{
	if (Barrier)
	{
		Barrier->SetClosed(bClosed);
	}
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

AGeoArena* AGeoArena::GetFightingArena(UObject const* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGeoArena> It(World); It; ++It)
	{
		if (It->bFighting)
		{
			return *It;
		}
	}
	return nullptr;
}

void AGeoArena::StartFight()
{
	if (!ensureMsgf(GeoLib::IsServer(this), TEXT("%s: StartFight is server-only"), *GetName()))
	{
		return;
	}
	AGeoGameState* GameState = GetWorld()->GetGameStateChecked<AGeoGameState>();
	if (GameState->IsMatchInProgress())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: StartFight ignored — another fight is already in progress"), *GetName());
		return;
	}

	bFighting = true;
	ApplyBossBar();

	GameState->SetCheckpointTag(ArenaTag);

	if (ensureMsgf(IsValid(Boss), TEXT("%s: StartFight with no boss to bind"), *GetName()))
	{
		Boss->OnEnemyDefeated.AddUniqueDynamic(this, &AGeoArena::OnBossDefeated);
	}

	SetBarrierClosed(true);

	GetWorld()->GetTimerManager().SetTimer(CommitFightTimer, this, &AGeoArena::CommitFight, GameState->CommitFightTime,
										   false);
}

void AGeoArena::CommitFight()
{
	GeoLib::TeleportPlayersToTargetPoints(this, FGeoGameplayTags::Get().TargetPoint_FightLocation, ArenaTag,
										  FightZoneTagName);
}

void AGeoArena::OnWipe(float /*DeathTime*/)
{
	if (!bFighting)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(CommitFightTimer);
	SetBarrierClosed(false);
}

void AGeoArena::EndFight()
{
	GetWorld()->GetTimerManager().ClearTimer(CommitFightTimer);
	bFighting = false;
	ApplyBossBar();
	SetBarrierClosed(false);
	// A defeated boss stays defeated: respawning it here would have it re-aggro the players still standing on its
	// corpse. Only RespawnBoss (a match-state button, or a fresh server) brings it back.
	if (!bBossDefeated)
	{
		ResetBoss();
	}
}

void AGeoArena::RespawnBoss()
{
	bBossDefeated = false;
	ResetBoss();
}

void AGeoArena::RespawnAllBosses(UObject const* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}
	for (TActorIterator<AGeoArena> It(World); It; ++It)
	{
		It->RespawnBoss();
	}
}

bool AGeoArena::IsBoss(AActor const* Enemy) const
{
	return IsValid(Enemy) && Enemy == Boss;
}

void AGeoArena::OnMatchStateChanged(FName NewMatchState, FName PreviousMatchState)
{
	if (NewMatchState == MatchState::InProgress)
	{
		StopLoot();
	}
	else if (PreviousMatchState == MatchState::InProgress)
	{
		EndFight();
	}
}

void AGeoArena::OnRep_bFighting()
{
	ApplyBossBar();
}

void AGeoArena::ApplyBossBar()
{
	APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();
	if (!LocalPlayerController)
	{
		return;
	}
	IGeoHUDInterface* GeoHUD = Cast<IGeoHUDInterface>(LocalPlayerController->GetHUD());
	if (!GeoHUD)
	{
		return;
	}
	if (bFighting && IsValid(Boss))
	{
		GeoHUD->ShowBossHealthBar(Boss);
	}
	else
	{
		GeoHUD->HideBossHealthBar();
	}
}

void AGeoArena::OnBossDefeated()
{
	// Capture before the boss destroys itself right after broadcasting its defeat.
	if (IsValid(Boss))
	{
		LootOrigin = Boss->GetActorLocation();
	}
	bBossDefeated = true;

	Loot();
	GetWorld()->GetGameStateChecked<AGeoGameState>()->RequestWaitingToStart();
}

void AGeoArena::Loot()
{
	if (!GeoLib::IsServer(this))
	{
		return;
	}

	// Resolve the Blueprint-derived reload ability CDO that owns the pickup config (class, buff pool, color palette)
	// once for the whole shower. The ability catalog is keyed by the Spell AbilityTag, which has no native constant,
	// so find the entry by class.
	LootReloadCDO = nullptr;
	LootReloadTag = FGameplayTag();
	if (UAbilityInfo const* AbilityInfo = GeoASLib::GetAbilityInfo())
	{
		for (FGameplayAbilityInfo const& Info : AbilityInfo->GetAllAbilityInfos())
		{
			if (Info.AbilityClass && Info.AbilityClass->IsChildOf(UGeoReloadAbility::StaticClass()))
			{
				LootReloadCDO = Info.AbilityClass->GetDefaultObject<UGeoReloadAbility>();
				LootReloadTag = Info.AbilityTag;
				break;
			}
		}
	}
	if (!ensureMsgf(LootReloadCDO && LootReloadCDO->BuffPickupClass,
					TEXT("%hs: no reload ability with a BuffPickupClass registered in AbilityInfo"), __FUNCTION__))
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(LootTimer, this, &AGeoArena::SpawnLootBurst, LootSpawnInterval, true);
}

void AGeoArena::StopLoot()
{
	GetWorld()->GetTimerManager().ClearTimer(LootTimer);
	for (TWeakObjectPtr<UGeoDeployableManagerComponent> const& Manager : LootBoostedManagers)
	{
		if (UGeoDeployableManagerComponent* DeployableManager = Manager.Get())
		{
			DeployableManager->RemoveDeployableSlot(LootPickupClass);
		}
	}
	LootBoostedManagers.Empty();
}

void AGeoArena::SpawnLootBurst()
{
	TArray<TInstancedStruct<FEffectData>> const BuffEffects = LootReloadCDO->GetEffectDataArray();

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
	// don't expire each other. Restored in StopLoot when the next fight starts.
	if (UGeoDeployableManagerComponent* DeployableManager =
			PayloadOwner->GetComponentByClass<UGeoDeployableManagerComponent>())
	{
		DeployableManager->SetDeployableInfinitCount(LootReloadCDO->BuffPickupClass);
		LootBoostedManagers.AddUnique(DeployableManager);
		LootPickupClass = LootReloadCDO->BuffPickupClass;
	}

	FAbilityPayload Payload;
	Payload.Origin = FVector2D(LootOrigin);
	Payload.ServerSpawnTime = GetWorld()->GetTimeSeconds();
	Payload.AbilityTag = LootReloadTag;
	Payload.Owner = PayloadOwner;
	Payload.Instigator = PayloadOwner;

	FTransform const SpawnTransform{LootOrigin};
	for (int32 PickupIndex = 0; PickupIndex < LootPickupsPerBurst; ++PickupIndex)
	{
		AGeoBuffPickup* Pickup = GetWorld()->SpawnActorDeferred<AGeoBuffPickup>(
			LootReloadCDO->BuffPickupClass, SpawnTransform, PayloadOwner, PayloadOwner,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!ensureMsgf(IsValid(Pickup), TEXT("%hs: failed to spawn AGeoBuffPickup"), __FUNCTION__))
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
