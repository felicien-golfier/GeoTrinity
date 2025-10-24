#include "InputStep.h"

FString FInputStep::ToString() const
{
	return FString::Printf(TEXT("Inputs :\n Movement : X %f, Y %f\n Ping : %f\n Time : %d,%f\n ServerTime: %f"), MovementInput.X,
		MovementInput.Y, Ping, TimeSeconds, TimePartialSeconds, ServerTimeSeconds);
}

double FInputStep::GetTimeDiff(const FInputStep& Other) const
{
	// Prefer server time delta if available on both steps
	if (ServerTimeSeconds > 0.0 && Other.ServerTimeSeconds > 0.0)
	{
		return ServerTimeSeconds - Other.ServerTimeSeconds;
	}
	// Fallback to local accurate real time components
	return static_cast<double>(TimeSeconds - Other.TimeSeconds) + TimePartialSeconds - Other.TimePartialSeconds;
}

bool FInputStep::IsEmpty() const
{
	const bool bMovementZero = MovementInput.IsNearlyZero(KINDA_SMALL_NUMBER);
	const bool bPingZero = FMath::IsNearlyZero(Ping);
	const bool bTimeSecondsZero = TimeSeconds == 0;
	const bool bTimePartialZero = FMath::IsNearlyZero(TimePartialSeconds);
	const bool bServerTimeZero = FMath::IsNearlyZero(ServerTimeSeconds);
	return bMovementZero && bPingZero && bTimeSecondsZero && bTimePartialZero && bServerTimeZero;
}

void FInputStep::Empty()
{
	MovementInput = FVector2D::ZeroVector;
	Ping = 0.f;
	TimeSeconds = 0;
	TimePartialSeconds = 0.0;
	ServerTimeSeconds = 0.0;
}
