// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/BombZone/GeoBombZone.h"

#include "Actor/GeoHexArena.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoBombZone::AGeoBombZone(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
{
	bUseRegularDrain = true;
	bAutoRecallAtEndLife = true;
	bExplodeAtRecall = true;
	bDestroyOldestWhenLimitReached = true;
	bSurviveOverTheVoid = true;
	SetCanBeDamaged(false);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AGeoBombZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GeoLib::IsServer(GetWorld()) && IsActive())
	{
		AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(BombData.Owner);
		if (ensureMsgf(Arena, TEXT("AGeoBombZone: owner %s is not a hex arena boss"), *GetNameSafe(BombData.Owner)))
		{
			Arena->HighlightTiles(this, FVector2D(GetActorLocation()),
								  GetData()->Params.Size); // Will be replicated in the Arena itself.
		}
	}
}

void AGeoBombZone::InitInteractable(FInteractableActorData* Data)
{
	FDeployableData* const DeployableData = static_cast<FDeployableData*>(Data);
	if (!ensureMsgf(DeployableData, TEXT("AGeoBombZone: Data is not a FDeployableData!")))
	{
		return;
	}
	BombData = *DeployableData;

	Super::InitInteractable(Data);
}

void AGeoBombZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGeoBombZone, BombData, COND_InitialOnly);
}

FGameplayCueParameters AGeoBombZone::GetSpawnCueParams(FGameplayTag SoundTag)
{
	FGameplayCueParameters CueParams = Super::GetSpawnCueParams(SoundTag);
	CueParams.SourceObject = this;
	return CueParams;
}

void AGeoBombZone::StartBlinking()
{
	Super::StartBlinking();

	if (GeoLib::IsServer(GetWorld()))
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(BombData.Owner);
		if (ensureMsgf(Arena, TEXT("AGeoBombZone: owner %s is not a hex arena boss"), *GetNameSafe(BombData.Owner)))
		{
			Arena->HighlightTiles(this, FVector2D(GetActorLocation()),
								  GetData()->Params.Size); // Will be replicated in the Arena itself.
		}
	}
}

void AGeoBombZone::ExplodeEffect(float const Value)
{
	Super::ExplodeEffect(Value);

	if (GeoLib::IsServer(GetWorld()))
	{
		AGeoHexArena* const Arena = AGeoHexArena::GetArenaOfBoss(BombData.Owner);
		if (ensureMsgf(Arena, TEXT("AGeoBombZone: owner %s is not a hex arena boss"), *GetNameSafe(BombData.Owner)))
		{
			Arena->DestroyTilesInRadius(FVector2D(GetActorLocation()), BombData.Params.Size);
			Arena->ClearHighlight(this);
		}
	}
}
