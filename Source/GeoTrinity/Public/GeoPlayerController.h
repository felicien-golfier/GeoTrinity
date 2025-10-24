// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"

#include "GeoPlayerController.generated.h"

/**
 *
 */
UCLASS()
class GEOTRINITY_API AGeoPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AGeoPlayerController(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* APawn) override;

public:
	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Time synchronization interface
	UFUNCTION(Server, unreliable)
	void ServerRequestServerTime(double ClientSendTimeSeconds);

	UFUNCTION(Client, unreliable)
	void ClientReportServerTime(double ClientSendTimeSeconds, double ServerTimeSeconds);

	// Returns client-side estimate of server time in seconds
	double GetEstimatedServerTimeSeconds() const;

private:
	void ScheduleTimeSync();
	void SendTimeSyncRequest();

private:
	// Smoothed offset such that: EstimatedServerTime = ClientRealTime + ServerTimeOffsetSeconds
	double ServerTimeOffsetSeconds = 0.0;
	bool bHasServerTimeOffset = false;
	FTimerHandle TimeSyncTimerHandle;

	UPROPERTY()
	FVector2D MovementInputs;
};
