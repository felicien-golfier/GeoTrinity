#pragma once

#include "CoreMinimal.h"
#include "GeoPawnState.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"

#include "GeoStateSubsystem.generated.h"

UCLASS()
class GEOTRINITY_API UGeoStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// UWorldSubsystem overrides
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// Helper to fetch the subsystem instance for a given world.
	static UGeoStateSubsystem* GetInstance(const UWorld* World);

	void ReceivedServerSnapshot(const FGeoGameSnapShot& Snapshot);
	void RollBackToTime(FGeoTime Time);

private:
	// Timer callback to capture and dispatch snapshot from server to clients
	void BroadcastAuthoritativeSnapshot();
	FGeoGameSnapShot GetCurrentSnapshot() const;
	// Apply a previously captured snapshot to restore pawns to a specific game state.
	void ApplySnapshot(const FGeoGameSnapShot& Snapshot);

private:
	// Repeating timer to send authoritative snapshots every 0.5s
	FTimerHandle SnapshotTimerHandle;
	FGeoGameSnapShot LastAppliedSnapshot;

	TArray<FGeoGameSnapShot> GameHistory;
};
