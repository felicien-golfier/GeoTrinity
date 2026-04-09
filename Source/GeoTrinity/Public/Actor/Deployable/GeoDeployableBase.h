// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoDeployableBase.generated.h"


struct FEffectData;
class UGeoCombattantWidgetComp;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeployableDestroyed, AGeoDeployableBase*, Deployable);

USTRUCT(Blueprintable)
struct FDeployableDataParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BlinkDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float LifeDrainMaxDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Size = 0.f;
};

USTRUCT()
struct FDeployableData : public FInteractableActorData
{
	GENERATED_BODY()

	UPROPERTY(Transient, NotReplicated)
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	UPROPERTY(Transient)
	FDeployableDataParams Params;
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
	virtual void InitDrain();
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** Called by the owning player to recall this deployable */
	virtual void OnRecalled();

	/** Returns health ratio (0..1). Returns 1 if no duration limit. */
	UFUNCTION(BlueprintPure)
	virtual float GetDurationPercent() const;
	UFUNCTION(BlueprintPure)
	bool IsExpired() const { return bExpired; }
	UFUNCTION(BlueprintPure)
	bool IsBlinking() const;

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

	virtual void OnHealthChanged_Implementation(float NewValue) override;

	/** Called when duration or health reaches zero */
	UFUNCTION(BlueprintNativeEvent)
	void OnDeployableExpired();
	virtual void OnDeployableExpired_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void OnBlinkStarted();
	virtual void OnBlinkStarted_Implementation();

	UPROPERTY(BlueprintReadOnly)
	bool bUseRegularDrain = true;
	UPROPERTY(BlueprintReadOnly)
	float DrainMagnitudePerSecond = 0.f;

private:
	void OnBlinkTimerExpired();
	void OnBlinkVisibilityTick();

	FTimerHandle BlinkTimerHandle;
	FTimerHandle BlinkVisibilityTimerHandle;
	bool bExpired = false;
};
