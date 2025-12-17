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

	UFUNCTION(Server, Reliable)
	void ServerSetAimYaw(float YawDegrees);

protected:
	virtual void BeginPlay() override;

private:
	// Time synchronization interface
	UFUNCTION(Client, unreliable)
	void SendToClientTheServerTime(double ServerTimeSeconds);

	UFUNCTION(Server, Reliable)
	void SendServerTimeOffsetToServer(float StabilizedServerTimeOffset);

	UFUNCTION()
	void RequestTimeSync();
	void CalculateStableServerTimeOffset();

public:
	bool HasServerTime() const { return bHasServerTime; };

	UFUNCTION(BlueprintCallable)
	double GetServerTime() const;
	static double GetServerTime(const UWorld* World);
	static AGeoPlayerController* GetLocalGeoPlayerController(const UWorld* World);

private:
	bool bHasServerTime = false;
	TArray<float> ServerTimeOffsetSamples;
	TArray<float> Pings;
	float ServerTimeOffset;
	FTimerHandle TimeSyncTimerHandle;
	static constexpr int32 NumSamplesToStabilize = 200;
	static constexpr float MaxDeviationFromMedian = 0.01f;

public:
	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
};
