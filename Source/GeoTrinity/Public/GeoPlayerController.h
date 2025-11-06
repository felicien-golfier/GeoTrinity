// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GeoPawnState.h"
#include "InputMappingContext.h"
#include "InputStep.h"

#include "GeoPlayerController.generated.h"

class UGeoInputConfig;
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
	void SetServerTimeOffsetSeconds(float InServerTimeOffsetSeconds);

public:
	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Time synchronization interface
	UFUNCTION(Client, unreliable)
	void RequestClientTime(FGeoTime ServerTimeSeconds);

	UFUNCTION(Server, unreliable)
	void ReportClientTime(FGeoTime ClientSendTimeSeconds, FGeoTime ServerTimeSeconds);

	UFUNCTION(Client, Reliable)
	void ClientSendServerTimeOffset(float ServerTimeOffset);

private:
	UFUNCTION()
	void SendTimeSyncRequest();
	bool IsServerTimeOffsetStable() const;

	TArray<float> ServerTimeOffsetSamples;
	static constexpr int32 NumSamplesToStabilize = 10;
	FTimerHandle TimeSyncTimerHandle;
	
	UPROPERTY()
	FVector2D MovementInputs;
};
