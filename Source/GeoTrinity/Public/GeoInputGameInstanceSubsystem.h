// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoPawn.h"
#include "InputStep.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GeoInputGameInstanceSubsystem.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API UGeoInputGameInstanceSubsystem
	: public UGameInstanceSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

	// FTickableGameObject implementation Begin
	ENGINE_API virtual UWorld* GetTickableGameObjectWorld() const override;
	ENGINE_API virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	ENGINE_API virtual void Tick(float DeltaTime) override;
	// FTickableGameObject implementation End

	void AddInputBuffer(const FInputStep& InputStep, AGeoPawn* GeoPawn);
	void ProcessAgents(const float DeltaTime);

	/** Returns true if Initialize has been called but Deinitialize has not */
	bool IsInitialized() const { return bInitialized; }

private:
	bool bInitialized = false;

	UPROPERTY()
	TMap<AGeoPawn*, FInputAgent> InputAgents;

	UPROPERTY(EditDefaultsOnly)
	int MaxBufferInputs = 50;
};
