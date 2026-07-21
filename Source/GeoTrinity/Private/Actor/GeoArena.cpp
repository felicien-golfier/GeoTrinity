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
#include "Engine/World.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HUD/Interface/GeoHUDInterface.h"
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
	Boss->Arena = this;

	if (Barrier)
	{
		Barrier->SetClosed(false);
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
	TArray<AActor*> Arenas;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject, AGeoArena::StaticClass(), Arenas);
	for (AActor* Actor : Arenas)
	{
		AGeoArena* Arena = CastChecked<AGeoArena>(Actor);
		if (Arena->bFighting)
		{
			return Arena;
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

	if (Barrier)
	{
		Barrier->SetClosed(true);
	}

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
	if (Barrier)
	{
		Barrier->SetClosed(false);
	}
}

void AGeoArena::EndFight()
{
	GetWorld()->GetTimerManager().ClearTimer(CommitFightTimer);
	bFighting = false;
	ApplyBossBar();
	ResetBoss();
}

bool AGeoArena::IsBoss(AActor const* Enemy) const
{
	return IsValid(Enemy) && Enemy == Boss;
}

void AGeoArena::OnMatchStateChanged(FName /*NewMatchState*/, FName /*PreviousMatchState*/)
{
	if (GetWorld()->GetGameStateChecked<AGeoGameState>()->IsMatchInProgress())
	{
		StopLoot();
	}
	else if (bFighting)
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

	Loot();
	GetWorld()->GetGameStateChecked<AGeoGameState>()->RequestWaitingToStart();
}

void AGeoArena::Loot()
{
	if (!GeoLib::IsServer(this))
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
	// don't expire each other. Restored in StopLoot when the next fight starts.
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
