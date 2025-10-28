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
	bool IsInitialized() const { return bInitialized; }

	// FTickableGameObject implementation Begin
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	// FTickableGameObject implementation End

	void ClientUpdateInputBuffer(const TArray<FInputAgent>& InputAgents);
	void ServerAddInputBuffer(const FInputStep& InputStep, AGeoPawn* GeoPawn);
	void ProcessAgents(const float DeltaTime);
	void UpdateClients();

	FInputAgent& GetInputAgent(AGeoPawn* GeoPawn);
	static UGeoInputGameInstanceSubsystem* GetInstance(const UWorld* World);

private:
	bool bInitialized = false;
	// Map of pawn -> buffered inputs received on the server
	UPROPERTY()
	TMap<AGeoPawn*, FInputAgent> InputAgentsHistory;
	UPROPERTY()
	TMap<AGeoPawn*, FInputAgent> NewInputAgents;

	UPROPERTY(EditDefaultsOnly)
	int MaxBufferInputs = 20;
};
