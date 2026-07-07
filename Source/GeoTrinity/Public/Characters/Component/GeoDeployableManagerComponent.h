// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GeoDeployableManagerComponent.generated.h"

class AGeoDeployableBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeployCountChanged, int32, CurrentCount, int32, MaxCount);

USTRUCT()
struct FDeployableBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AGeoDeployableBase>> Deployables;
};

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
	/**
	 * Returns true if a new deployable of the given class may be deployed.
	 * Always returns true when the class has bDestroyOldestWhenLimitReached set
	 * (the oldest will be expired on RegisterDeployable instead of blocking).
	 * Otherwise delegates to HasReachMaxLimit.
	 */
	bool CanDeploy(TSubclassOf<AGeoDeployableBase> DeployableClass);

	/**
	 * Returns true if another deployable of the given class can be deployed.
	 * If the class has an entry in DeployableSlots, that per-class cap is checked.
	 * Otherwise falls back to the global MaxDeployables limit.
	 */
	UFUNCTION(BlueprintPure)
	bool HasReachMaxLimit(TSubclassOf<AGeoDeployableBase> DeployableClass);

	/** Returns the configured maximum number of simultaneous deployables. */
	UFUNCTION(BlueprintPure)
	int32 GetMaxDeployables() const { return MaxDeployables; }

	/** Register a newly deployed actor. Binds to its destroyed delegate. */
	void RegisterDeployable(AGeoDeployableBase* Deployable);

	/** Immediately expires all tracked deployables. Used on character EndPlay and class switch. */
	void ForceExpireAll() const;

	/** AActor* delegate callback bound to AGeoDeployableBase::OnDestroyed; forwards to the typed overload. */
	void OnDeployableDestroyed(AActor* Deployable);

	/** Returns all live tracked deployable actors. */
	TArray<AGeoDeployableBase*> GetAllDeployables() const;

	/**
	 * Returns all live tracked deployable actors whose class is T or a subclass of T, cast to T*.
	 * Iterates every bucket whose stored class IsChildOf T; skips buckets for unrelated classes.
	 */
	template <typename T>
	TArray<T*> GetDeployables() const
	{
		TArray<T*> Result;
		for (auto const& [StoredClass, Bucket] : Deployables)
		{
			if (StoredClass->IsChildOf(T::StaticClass()))
			{
				for (AGeoDeployableBase* Deployable : Bucket.Deployables)
				{
					Result.Add(CastChecked<T>(Deployable));
				}
			}
		}
		return Result;
	}

	/** Adds a DeployableSlots entry for Class with limit 0, granting unlimited deployments of that class. */
	void SetDeployableInfinitCount(TSubclassOf<AGeoDeployableBase> Class);
	/** Removes the DeployableSlots entry for Class, putting it back on the global MaxDeployables pool. */
	void RemoveDeployableSlot(TSubclassOf<AGeoDeployableBase> Class);
	/** Purges null or pending-kill entries from Bucket.Deployables in place. */
	void RemoveInvalidDeployables(FDeployableBucket& Bucket);

	UPROPERTY(BlueprintAssignable)
	FOnDeployCountChanged OnDeployCountChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable", meta = (ClampMin = "1"))
	int32 MaxDeployables = 3;

	/**
	 * Per-class deployable slots. Add an entry to give a class its own independent cap.
	 * Classes without an entry share the global MaxDeployables pool.
	 * Set class to 0 if you need infinit deployables.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deployable")
	TMap<TSubclassOf<AGeoDeployableBase>, int32> DeployableSlots;

private:
	UFUNCTION()
	void OnDeployableDestroyed(AGeoDeployableBase* Deployable);

	UPROPERTY()
	TMap<TSubclassOf<AGeoDeployableBase>, FDeployableBucket> Deployables;
};
