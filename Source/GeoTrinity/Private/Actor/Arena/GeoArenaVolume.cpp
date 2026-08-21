// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Arena/GeoArenaVolume.h"

#include "Characters/PlayableCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameClasses/GeoGameState.h"
#include "GameFramework/Pawn.h"
#include "Tool/UGeoGameplayLibrary.h"

AGeoArenaVolume::AGeoArenaVolume()
{
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(500.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AGeoArenaVolume::BeginPlay()
{
	Super::BeginPlay();
	if (!GeoLib::IsServer(this))
	{
		return;
	}
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnBeginOverlap);

	// Overlaps are resolved when the box registers, before this binding exists, so a player already standing here
	// never fires it.
	TArray<AActor*> PlayersInside;
	TriggerBox->GetOverlappingActors(PlayersInside, APlayableCharacter::StaticClass());
	if (!PlayersInside.IsEmpty())
	{
		ClaimCurrentArena();
	}
}

void AGeoArenaVolume::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
									 UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
									 FHitResult const& /*SweepResult*/)
{
	if (Cast<APlayableCharacter>(OtherActor))
	{
		ClaimCurrentArena();
	}
}

void AGeoArenaVolume::ClaimCurrentArena() const
{
	AGeoGameState* GameState = GetWorld()->GetGameStateChecked<AGeoGameState>();
	if (!GameState->IsMatchInProgress())
	{
		GameState->SetCurrentArenaTag(ArenaTag);
	}
}

bool AGeoArenaVolume::IsPawnInside(UObject const* WorldContextObject, APawn const& Pawn, FGameplayTag const ArenaTag)
{
	for (TActorIterator<AGeoArenaVolume> It(WorldContextObject->GetWorld()); It; ++It)
	{
		if (It->ArenaTag == ArenaTag && Pawn.IsOverlappingActor(*It))
		{
			return true;
		}
	}
	return false;
}
