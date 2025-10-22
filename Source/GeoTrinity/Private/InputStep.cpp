#include "InputStep.h"

FString FInputStep::ToString() const
{
	return FString::Printf(TEXT("Inputs : \n Movement : X %f, Y %f \n Ping : %f \n Time : %d,%f"), MovementInput.X, MovementInput.Y, Ping,
		TimeSeconds, TimePartialSeconds);
}

double FInputStep::GetTimeDiff(const FInputStep& Other) const
{

	return static_cast<double>(TimeSeconds - Other.TimeSeconds) + TimePartialSeconds - Other.TimePartialSeconds;
}

bool FInputStep::IsEmpty() const
{
	const bool bMovementZero = MovementInput.IsNearlyZero(KINDA_SMALL_NUMBER);
	const bool bPingZero = FMath::IsNearlyZero(Ping);
	const bool bTimeSecondsZero = TimeSeconds == 0;
	const bool bTimePartialZero = FMath::IsNearlyZero(TimePartialSeconds);
	return bMovementZero && bPingZero && bTimeSecondsZero && bTimePartialZero;
}

void FInputStep::Empty()
{
	MovementInput = FVector2D::ZeroVector;
	Ping = 0.f;
	TimeSeconds = 0;
	TimePartialSeconds = 0.0;
}
