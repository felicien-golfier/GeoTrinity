#include "GeoStateSubsystem.h"

#include "Engine/World.h"
#include "GeoPawn.h"
#include "GeoPawnState.h"
#include "GeoPlayerController.h"

FGeoGameSnapShot UGeoStateSubsystem::GetCurrentSnapshot() const
{
	FGeoGameSnapShot CurrentSnapShot;
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

		if (GeoPlayerController->IsLocalController())
		{
			CurrentSnapShot.ServerTime = GeoPlayerController->GetHestimatedServerTime();
		}

		FGeoPawnState GeoPawnState{GeoPawn, GeoPawn->GetActorLocation(), GeoPawn->GetActorRotation(), GeoPawn->GetVelocity()};
		CurrentSnapShot.GeoPawnStates.Add(GeoPawnState);
	}

	return CurrentSnapShot;
}

void UGeoStateSubsystem::ApplySnapshot(const FGeoGameSnapShot& Snapshot)
{
	// Restore each pawn state found in the snapshot
	for (const FGeoPawnState& PawnState : Snapshot.GeoPawnStates)
	{
		AGeoPawn* Pawn = PawnState.GeoPawn.Get();
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

void UGeoStateSubsystem::RollBackToLastAppliedSnapshot()
{
	ApplySnapshot(LastAppliedSnapshot);
}

UGeoStateSubsystem* UGeoStateSubsystem::GetInstance(const UWorld* World)
{
	if (!IsValid(World))
	{
		return nullptr;
	}

	return World->GetSubsystem<UGeoStateSubsystem>();
}
