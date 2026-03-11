#pragma once

#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"

#include "GeoDeployableBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeployableDestroyed, AGeoDeployableBase*, Deployable);

USTRUCT()
struct FDeployableData : public FInteractableActorData
{
	GENERATED_BODY()

	UPROPERTY()
	float MaxDuration = 0.f;
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

	/** Called by the owning player to recall this deployable */
	virtual void OnRecalled();

	/** Returns remaining duration as 0..1 (1 = full). Returns 1 if no duration limit. */
	float GetDurationPercent() const;
	bool IsExpired() const { return bExpired; }

	virtual void InitInteractableData(FInteractableActorData* Data) override;
	UPROPERTY(BlueprintAssignable)
	FOnDeployableDestroyed OnDeployableDestroyed;

protected:
	virtual FDeployableData const* GetData() const override
	{
		checkNoEntry();
		return nullptr;
	}

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHealthChanged(float NewValue) override;

	/** Called when duration or health reaches zero */
	virtual void OnDeployableExpired();

private:
	float RemainingDuration = 0.f;
	bool bExpired = false;
};
