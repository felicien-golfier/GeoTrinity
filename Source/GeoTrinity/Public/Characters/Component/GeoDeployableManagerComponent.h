#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoDeployableManagerComponent.generated.h"

class AGeoDeployableBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeployCountChanged, int32, CurrentCount, int32, MaxCount);

/**
 * Tracks deployed actors for a player. Manages max count, handles recall,
 * and broadcasts when count changes (used for size scaling effects).
 */
UCLASS(ClassGroup = "GeoTrinity", meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoDeployableManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGeoDeployableManagerComponent();

	/** Returns true if under the max limit */
	bool CanDeploy() const { return Deployables.Num() < MaxDeployables; }
	int32 GetDeployedCount() const { return Deployables.Num(); }
	int32 GetMaxDeployables() const { return MaxDeployables; }

	/** Register a newly deployed actor. Binds to its destroyed delegate. */
	void RegisterDeployable(AGeoDeployableBase* Deployable);

	/** Recall all deployed actors */
	void RecallAll();

	/** Recall the oldest deployed actor */
	void RecallOldest();

	/** Get the ratio of deployed/max (used for size scaling) */
	float GetDeployRatio() const;

	UPROPERTY(BlueprintAssignable)
	FOnDeployCountChanged OnDeployCountChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Deployable", meta = (ClampMin = "1"))
	int32 MaxDeployables = 3;

private:
	UFUNCTION()
	void OnDeployableDestroyed(AGeoDeployableBase* Deployable);

	UPROPERTY()
	TArray<TObjectPtr<AGeoDeployableBase>> Deployables;
};
