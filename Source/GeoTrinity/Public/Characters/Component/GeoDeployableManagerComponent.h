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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Returns true if under the max limit */
	UFUNCTION(BlueprintPure)
	bool CanDeploy() const { return Deployables.Num() < MaxDeployables; }
	UFUNCTION(BlueprintPure)
	int32 GetDeployedCount() const { return Deployables.Num(); }
	UFUNCTION(BlueprintPure)
	int32 GetMaxDeployables() const { return MaxDeployables; }

	/** Register a newly deployed actor. Binds to its destroyed delegate. */
	void RegisterDeployable(AGeoDeployableBase* Deployable);

	/** Recall all deployed actors */
	void RecallAll();

	/** Get the ratio of deployed/max (used for size scaling) */
	UFUNCTION(BlueprintPure)
	float GetDeployRatio() const;

	TArray<TObjectPtr<AGeoDeployableBase>> const& GetDeployables() const { return Deployables; }

	UPROPERTY(BlueprintAssignable)
	FOnDeployCountChanged OnDeployCountChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (ClampMin = "1"))
	int32 MaxDeployables = 3;

private:
	UFUNCTION()
	void OnDeployableDestroyed(AGeoDeployableBase* Deployable);

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AGeoDeployableBase>> Deployables;
};
