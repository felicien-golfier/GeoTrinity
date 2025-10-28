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
	// Comparison operators
	bool operator<(const FGeoTime& Other) const;
	bool operator>(const FGeoTime& Other) const;
	// Subtract another time; returns difference in seconds (this - Other)
	double operator-(const FGeoTime& Other) const;
	// Subtract a number of seconds (can be fractional) from this time and return the result
	FGeoTime operator-(double DeltaSeconds) const;
	// Add a number of seconds (can be fractional) to this time and return the result
	FGeoTime operator+(double DeltaSeconds) const;
	// Equality comparison with another FGeoTime (with tolerance)
	bool operator==(const FGeoTime& Other) const;
	// Utility: check whether time is effectively zero
	bool IsZero(double Tolerance = 1e-6) const;
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

	FInputStep() : MovementInput(FVector2D()), DeltaTimeSeconds(0.f), Time(FGeoTime()), ServerTimeOffsetSeconds(0.0) {}
	FInputStep(FVector2D InMovementInput, float InDeltaTimeSeconds, int32 InTimeSeconds, double InTimePartialSeconds)
		: MovementInput(InMovementInput)
		, DeltaTimeSeconds(InDeltaTimeSeconds)
		, Time(FGeoTime(InTimeSeconds, InTimePartialSeconds))
		, ServerTimeOffsetSeconds(0.0)
	{
	}

	FString ToString() const;
	double GetTimeDiff(const FInputStep& Other) const;
	bool IsEmpty() const;
	void Empty();
	FGeoTime ServerTime() const;

	UPROPERTY()
	FVector2D MovementInput;
	UPROPERTY()
	float DeltaTimeSeconds;
	UPROPERTY()
	FGeoTime Time;
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
};
