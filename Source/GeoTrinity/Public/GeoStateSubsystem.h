#pragma once

#include "CoreMinimal.h"
#include "GeoPawnState.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GeoStateSubsystem.generated.h"

UCLASS()
class GEOTRINITY_API UGeoStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	void RollBackToLastAppliedSnapshot();
	// Helper to fetch the subsystem instance for a given world.
	static UGeoStateSubsystem* GetInstance(const UWorld* World);

private:
	FGeoGameSnapShot GetCurrentSnapshot() const;
	// Apply a previously captured snapshot to restore pawns to a specific game state.
	void ApplySnapshot(const FGeoGameSnapShot& Snapshot);

private:
	FGeoGameSnapShot LastAppliedSnapshot;
};
