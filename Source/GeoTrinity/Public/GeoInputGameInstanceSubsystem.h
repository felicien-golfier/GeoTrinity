// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeoPawn.h"
#include "GeoPlayerController.h"
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

	void ClientUpdateInputAgents(const TArray<FInputAgent>& InputAgents);
	void AddNewInput(const FInputStep& InputStep, AGeoPawn* GeoPawn);
	void ProcessAgents(const float DeltaTime);
	void UpdateClients();

	FInputAgent& GetInputAgent(AGeoPawn* GeoPawn);
	void SetServerTimeOffset(AGeoPlayerController* GeoPlayerController, float ServerTimeOffset);
	static bool HasLocalServerTimeOffset(const UWorld* World);
	bool HasLocalServerTimeOffset() const;
	bool HasServerTimeOffset(const AGeoPlayerController* GeoPlayerController) const;
	FGeoTime GetServerTime(const AGeoPlayerController* GeoPlayerController);

	static UGeoInputGameInstanceSubsystem* GetInstance(const UWorld* World);

private:
	bool bInitialized = false;
	UPROPERTY()
	TArray<FInputAgent> InputAgentsHistory;
	UPROPERTY()
	TArray<FInputAgent> NewInputAgents;

	UPROPERTY()
	TMap<AGeoPlayerController*, float> ServerTimeOffsets;
};
