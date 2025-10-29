#include "GeoStateSubsystem.h"

#include "Engine/World.h"
#include "GeoInputGameInstanceSubsystem.h"
#include "GeoPawn.h"
#include "GeoPawnState.h"
#include "GeoPlayerController.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Stats/Stats.h"
#include "TimerManager.h"

FGeoGameSnapShot UGeoStateSubsystem::GetCurrentSnapshot() const
{
	FGeoGameSnapShot CurrentSnapShot;
	CurrentSnapShot.ServerTime = FGeoTime::GetAccurateRealTime();   // not overriden on the server

	// Parse existing player controllers and create a snapshot from each pawn
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AGeoPlayerController* GeoPlayerController = Cast<AGeoPlayerController>(Iterator->Get());

		if (!IsValid(GeoPlayerController))
		{
			continue;
		}

		AGeoPawn* GeoPawn = Cast<AGeoPawn>(GeoPlayerController->GetPawn());
		if (!IsValid(GeoPawn))
		{
			continue;
		}

		if (GeoPlayerController->IsLocalController())   // override ServerTime if on the client
		{
			CurrentSnapShot.ServerTime =
				UGeoInputGameInstanceSubsystem::GetInstance(GetWorld())->GetServerTime(GeoPlayerController);
		}

		FGeoPawnState GeoPawnState{GeoPawn, GeoPawn->GetActorLocation(), GeoPawn->GetActorRotation(),
			GeoPawn->GetVelocity()};
		CurrentSnapShot.GeoPawnStates.Add(GeoPawnState);
	}

	return CurrentSnapShot;
}

void UGeoStateSubsystem::ApplySnapshot(const FGeoGameSnapShot& Snapshot)
{
	// Restore each pawn state found in the snapshot
	for (const FGeoPawnState& PawnState : Snapshot.GeoPawnStates)
	{
		AGeoPawn* Pawn = PawnState.GeoPawn;
		if (!IsValid(Pawn))
		{
			continue;
		}

		// Set transform (location and rotation). We ignore scale.
		Pawn->SetActorLocation(PawnState.Location, false, nullptr, ETeleportType::TeleportPhysics);
		Pawn->SetActorRotation(PawnState.Orientation, ETeleportType::TeleportPhysics);

		// Keep the pawn's internal box in sync with actor location if accessible
		// Note: GetBox returns a proxy in this project that allows updating Position
		Pawn->GetBox().Position = FVector2D(PawnState.Location);
	}

	LastAppliedSnapshot = Snapshot;
}

void UGeoStateSubsystem::RollBackToTime(const FGeoTime Time)
{
	for (FGeoGameSnapShot& Snapshot : GameHistory)
	{
		if (Snapshot.ServerTime > Time)
		{
			ApplySnapshot(Snapshot);
			return;
		}
	}
}

void UGeoStateSubsystem::ReceivedServerSnapshot(const FGeoGameSnapShot& Snapshot)
{
	ApplySnapshot(Snapshot);
}

UGeoStateSubsystem* UGeoStateSubsystem::GetInstance(const UWorld* World)
{
	if (!IsValid(World))
	{
		return nullptr;
	}

	return World->GetSubsystem<UGeoStateSubsystem>();
}

void UGeoStateSubsystem::Tick(float DeltaTime)
{
	// Do not tick until We have a server synch.
	if (GetWorld()->GetNetMode() == NM_Client && !UGeoInputGameInstanceSubsystem::HasLocalServerTimeOffset(GetWorld()))
	{
		return;
	}

	// Capture a snapshot every frame and store it in history for rollback/replay
	GameHistory.Add(GetCurrentSnapshot());
	if (GameHistory.Num() > MaxBufferInputs)
	{
		GameHistory.RemoveAt(0, GameHistory.Num() - MaxBufferInputs);
	}
}

TStatId UGeoStateSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoStateSubsystem, STATGROUP_Tickables);
}
