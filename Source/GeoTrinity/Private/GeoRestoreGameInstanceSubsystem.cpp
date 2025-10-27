#include "GeoRestoreGameInstanceSubsystem.h"

#include "GeoPawn.h"
#include "GeoPawnState.h"
#include "Engine/World.h"
#include "GameFramework/PawnMovementComponent.h"

void UGeoRestoreGameInstanceSubsystem::ApplySnapshot(const FGeoGameSnapShot& Snapshot)
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

		// If pawn has a movement component, try to set velocity for immediate state consistency
		if (UPawnMovementComponent* MoveComp = Pawn->GetMovementComponent())
		{
			MoveComp->Velocity = PawnState.Velocity;
		}
	}
}

UGeoRestoreGameInstanceSubsystem* UGeoRestoreGameInstanceSubsystem::GetInstance(UWorld* World)
{
	if (!IsValid(World))
	{
		return nullptr;
	}
	if (UGameInstance* GI = World->GetGameInstance())
	{
		return GI->GetSubsystem<UGeoRestoreGameInstanceSubsystem>();
	}
	return nullptr;
}
