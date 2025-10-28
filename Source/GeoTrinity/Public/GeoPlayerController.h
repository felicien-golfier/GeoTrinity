// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GeoPawnState.h"
#include "InputMappingContext.h"
#include "InputStep.h"

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
	void ServerRequestServerTime(FGeoTime ClientSendTimeSeconds);

	UFUNCTION(Client, unreliable)
	void ClientReportServerTime(FGeoTime ClientSendTimeSeconds, FGeoTime ServerTimeSeconds);

	// Receive authoritative snapshot from server every 0.5s
	UFUNCTION(Client, unreliable)
	void ClientReceiveSnapshot(const FGeoGameSnapShot& Snapshot);

	// Returns client-side estimate of server time in seconds
	double GetServerTimeOffsetSeconds() const;
	FGeoTime GetHestimatedServerTime() const;

private:
	void ScheduleTimeSync();
	void SendTimeSyncRequest();

	double ServerTimeOffsetSeconds = 0.0;
	FTimerHandle TimeSyncTimerHandle;

	UPROPERTY()
	FVector2D MovementInputs;
};
