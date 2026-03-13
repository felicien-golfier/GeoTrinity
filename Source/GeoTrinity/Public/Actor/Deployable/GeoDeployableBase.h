// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"

#include "GeoDeployableBase.generated.h"

class UGeoCombattantWidgetComp;

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
	virtual void BeginPlay() override;

	/** Called by the owning player to recall this deployable */
	virtual void OnRecalled();

	/** Returns health ratio (0..1). Returns 1 if no duration limit. */
	float GetDurationPercent() const;
	bool IsExpired() const { return bExpired; }

	virtual void InitInteractableData(FInteractableActorData* Data) override;
	UPROPERTY(BlueprintAssignable)
	FOnDeployableDestroyed OnDeployableDestroyed;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGeoCombattantWidgetComp> HealthBarComponent;

protected:
	virtual FDeployableData const* GetData() const override
	{
		checkNoEntry();
		return nullptr;
	}

	virtual void OnHealthChanged(float NewValue) override;

	/** Called when duration or health reaches zero */
	virtual void OnDeployableExpired();

private:
	bool bExpired = false;
};
