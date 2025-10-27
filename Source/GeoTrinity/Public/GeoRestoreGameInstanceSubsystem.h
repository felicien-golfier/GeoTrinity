#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GeoPawnState.h"
#include "GeoRestoreGameInstanceSubsystem.generated.h"

UCLASS()
class GEOTRINITY_API UGeoRestoreGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// Apply a previously captured snapshot to restore pawns to a specific game state.
	void ApplySnapshot(const FGeoGameSnapShot& Snapshot);

	// Helper to fetch the subsystem instance for a given world.
	static UGeoRestoreGameInstanceSubsystem* GetInstance(UWorld* World);
};
