#pragma once

#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"

#include "GeoDeployableBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeployableDestroyed, AGeoDeployableBase*, Deployable);

struct FDeployableData : FInteractableActorData
{
	float MaxDuration = 0.f; // 0 = infinite
};

/**
 * Base class for all deployable actors (turrets, walls, healing zones).
 * Replicated actors — spawned by the server and destroyed when expired or recalled.
 */
UCLASS(Abstract)
class GEOTRINITY_API AGeoDeployableBase : public AGeoInteractableActor
{
	GENERATED_BODY()

public:
	AGeoDeployableBase();

	void InitDeployableData(FDeployableData* Data);

	/** Called by the owning player to recall this deployable */
	virtual void OnRecalled();

	/** Returns remaining duration as 0..1 (1 = full). Returns 1 if no duration limit. */
	float GetDurationPercent() const;
	bool IsExpired() const { return bExpired; }

	UPROPERTY(BlueprintAssignable)
	FOnDeployableDestroyed OnDeployableDestroyed;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHealthChanged(float NewValue) override;

	/** Called when duration or health reaches zero */
	virtual void OnDeployableExpired();

	FDeployableData* DeployableData = nullptr;

private:
	float RemainingDuration = 0.f;
	bool bExpired = false;
};
