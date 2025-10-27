#pragma once

#include "CoreMinimal.h"

#include "InputStep.generated.h"

class AGeoPawn;

USTRUCT()
struct FGeoTime
{
	GENERATED_BODY()

	FGeoTime() : TimeSeconds(0), TimePartialSeconds(0.f) {}
	FGeoTime(int32 InTimeSeconds, double InTimePartialSeconds) : TimeSeconds(InTimeSeconds), TimePartialSeconds(InTimePartialSeconds) {}

	double GetTimeDiff(FGeoTime Other) const;
	// Subtract a number of seconds (can be fractional) from this time and return the result
	FGeoTime operator-(double DeltaSeconds) const;
	// Add a number of seconds (can be fractional) to this time and return the result
	FGeoTime operator+(double DeltaSeconds) const;
	static FGeoTime GetAccurateRealTime();

	UPROPERTY()
	int32 TimeSeconds;
	UPROPERTY()
	double TimePartialSeconds;
};

USTRUCT()
struct FInputStep
{
	GENERATED_BODY()

	FInputStep() : MovementInput(FVector2D()), Ping(0.f), InputTime(FGeoTime()), ServerTimeOffsetSeconds(0.0) {}
	FInputStep(FVector2D InMovementInput, float InPing, int32 InTimeSeconds, double InTimePartialSeconds)
		: MovementInput(InMovementInput)
		, Ping(InPing)
		, InputTime(FGeoTime(InTimeSeconds, InTimePartialSeconds))
		, ServerTimeOffsetSeconds(0.0)
	{
	}

	FString ToString() const;
	double GetTimeDiff(const FInputStep& Other) const;
	bool IsEmpty() const;
	void Empty();
	double GetEstimatedServerTimeSeconds() const;

	UPROPERTY()
	FVector2D MovementInput;
	UPROPERTY()
	float Ping;
	UPROPERTY()
	FGeoTime InputTime;
	// Estimated server time (in seconds, double). 0 when unknown on the client.
	UPROPERTY()
	double ServerTimeOffsetSeconds;
};

USTRUCT()
struct FInputAgent
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInputStep> InputSteps;
	UPROPERTY()
	TWeakObjectPtr<AGeoPawn> Owner;
	UPROPERTY()
	FInputStep LastProcessedInputStep;
};
