// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/GeoEncounterSubsystem.h"

#include "Encounter/GeoEncounter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UGeoEncounterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddLambda(
		[this](UWorld* const TornDownWorld)
		{
			if (TornDownWorld == GetWorld())
			{
				AbortEncounters();
			}
		});

#if WITH_EDITOR
	PIEEndedHandle = FWorldDelegates::OnPIEEnded.AddLambda(
		[this](UGameInstance* const EndedGameInstance)
		{
			if (EndedGameInstance == GetWorld()->GetGameInstance())
			{
				AbortEncounters();
			}
		});
#endif
}

void UGeoEncounterSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
#if WITH_EDITOR
	FWorldDelegates::OnPIEEnded.Remove(PIEEndedHandle);
#endif

	Super::Deinitialize();
}

void UGeoEncounterSubsystem::Raise(FGameplayTag const Tag, FGeoEncounterPayload Payload)
{
	Payload.Tag = Tag;
	OnRaised.Broadcast(Payload);
}

void UGeoEncounterSubsystem::AbortEncounters()
{
	if (!bSessionEnding)
	{
		bSessionEnding = true;
		for (TActorIterator<AGeoEncounter> It(GetWorld()); It; ++It)
		{
			It->Abort();
		}
	}
}
