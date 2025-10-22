#pragma once

#include "InputStep.generated.h"

class AGeoPawn;
USTRUCT()
struct FInputStep
{
	GENERATED_BODY()

	FInputStep() : MovementInput(FVector2D()), Ping(0.f), TimeSeconds(0), TimePartialSeconds(0.f) {}
	FInputStep(FVector2D InMovementInput, float InPing, int32 InTimeSeconds, double InTimePartialSeconds)
		: MovementInput(InMovementInput)
		, Ping(InPing)
		, TimeSeconds(InTimeSeconds)
		, TimePartialSeconds(InTimePartialSeconds)
	{
	}

	FString ToString() const;
	double GetTimeDiff(const FInputStep& Other) const;
	bool IsEmpty() const;
	void Empty();

	UPROPERTY()
	FVector2D MovementInput;
	UPROPERTY()
	float Ping;
	UPROPERTY()
	int32 TimeSeconds;
	UPROPERTY()
	double TimePartialSeconds;
};

USTRUCT()
struct FInputAgent
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInputStep> InputSteps;
	UPROPERTY()
	TObjectPtr<AGeoPawn> Owner;
};