#include "InputStep.h"

#include "Kismet/GameplayStatics.h"

double FGeoTime::GetTimeDiff(const FGeoTime Other) const
{
	return static_cast<double>(TimeSeconds - Other.TimeSeconds) + TimePartialSeconds - Other.TimePartialSeconds;
}

double FGeoTime::operator-(const FGeoTime& Other) const
{
	return GetTimeDiff(Other);
}

bool FGeoTime::operator<(const FGeoTime& Other) const
{
	if (TimeSeconds != Other.TimeSeconds)
	{
		return TimeSeconds < Other.TimeSeconds;
	}
	return TimePartialSeconds < Other.TimePartialSeconds;
}

bool FGeoTime::operator>(const FGeoTime& Other) const
{
	if (TimeSeconds != Other.TimeSeconds)
	{
		return TimeSeconds > Other.TimeSeconds;
	}
	return TimePartialSeconds > Other.TimePartialSeconds;
}

FGeoTime FGeoTime::operator-(double DeltaSeconds) const
{
	// Break DeltaSeconds into whole and fractional parts using floor so that fraction is in [0,1]
	const int32 Whole = FMath::FloorToInt(DeltaSeconds);
	const double Fraction = DeltaSeconds - static_cast<double>(Whole);

	int32 NewSeconds = TimeSeconds - Whole;
	double NewPartial = TimePartialSeconds - Fraction;

	// Normalize partial seconds into [0,1) and adjust seconds accordingly
	while (NewPartial < 0.0)
	{
		NewPartial += 1.0;
		--NewSeconds;
	}
	while (NewPartial >= 1.0)
	{
		NewPartial -= 1.0;
		++NewSeconds;
	}

	return FGeoTime(NewSeconds, NewPartial);
}

FGeoTime FGeoTime::operator+(double DeltaSeconds) const
{
	// Reuse subtraction by adding negative seconds
	return (*this) - (-DeltaSeconds);
}

bool FGeoTime::operator==(const FGeoTime& Other) const
{
	if (TimeSeconds != Other.TimeSeconds)
	{
		return false;
	}
	return FMath::IsNearlyEqual(TimePartialSeconds, Other.TimePartialSeconds, 1e-6);
}

bool FGeoTime::IsZero(double Tolerance) const
{
	return TimeSeconds == 0 && FMath::Abs(TimePartialSeconds) <= Tolerance;
}

FGeoTime FGeoTime::GetAccurateRealTime()
{
	int32 Secs = 0;
	double Part = 0.0;
	UGameplayStatics::GetAccurateRealTime(Secs, Part);
	return FGeoTime(Secs, Part);
}

FString FInputStep::ToString() const
{
	return FString::Printf(TEXT("Inputs :\n Movement : X %f, Y %f\n DeltaTime : %f\n Time : %d and %f \n ServerTimeOffsetSeconds: %f"),
		MovementInput.X, MovementInput.Y, DeltaTimeSeconds, Time.TimeSeconds, Time.TimePartialSeconds, ServerTimeOffsetSeconds);
}

double FInputStep::GetTimeDiff(const FInputStep& Other) const
{
	double TimeDiff = Time.GetTimeDiff(Other.Time);

	// Prefer server time delta if available on both steps
	if (ServerTimeOffsetSeconds > 0.0 && Other.ServerTimeOffsetSeconds > 0.0)
	{
		TimeDiff += ServerTimeOffsetSeconds - Other.ServerTimeOffsetSeconds;
	}

	return TimeDiff;
}

bool FInputStep::IsEmpty() const
{
	const bool bMovementZero = MovementInput.IsNearlyZero(KINDA_SMALL_NUMBER);
	const bool bDeltaTimeSecondsZero = FMath::IsNearlyZero(DeltaTimeSeconds);
	const bool bTimeSecondsZero = Time.TimeSeconds == 0;
	const bool bTimePartialZero = FMath::IsNearlyZero(Time.TimePartialSeconds);
	const bool bServerTimeZero = FMath::IsNearlyZero(ServerTimeOffsetSeconds);
	return bMovementZero && bDeltaTimeSecondsZero && bTimeSecondsZero && bTimePartialZero && bServerTimeZero;
}

void FInputStep::Empty()
{
	MovementInput = FVector2D::ZeroVector;
	DeltaTimeSeconds = 0.f;
	Time.TimeSeconds = 0;
	Time.TimePartialSeconds = 0.0;
	ServerTimeOffsetSeconds = 0.0;
}

FGeoTime FInputStep::ServerTime() const
{
	return Time + ServerTimeOffsetSeconds;
}