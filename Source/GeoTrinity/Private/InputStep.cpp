#include "InputStep.h"

#include "Kismet/GameplayStatics.h"

FString FGeoTime::ToString() const
{
	return FString::Printf(TEXT("%d.%06d"), TimeSeconds, FMath::RoundToInt32(TimePartialSeconds * 1000000.0f));
}

double FGeoTime::GetTimeDiff(const FGeoTime Other) const
{
	return static_cast<double>(TimeSeconds - Other.TimeSeconds) + TimePartialSeconds - Other.TimePartialSeconds;
}

double FGeoTime::operator-(const FGeoTime& Other) const
{
	return GetTimeDiff(Other);
}

bool FGeoTime::operator>=(const FGeoTime& Other) const
{
	return *this > Other || *this == Other;
}
bool FGeoTime::operator<=(const FGeoTime& Other) const
{
	return *this < Other || *this == Other;
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

// Compound operators implementations
FGeoTime& FGeoTime::operator+=(double DeltaSeconds)
{
	// Use existing operator+
	FGeoTime Tmp = (*this) + DeltaSeconds;
	TimeSeconds = Tmp.TimeSeconds;
	TimePartialSeconds = Tmp.TimePartialSeconds;
	return *this;
}

FGeoTime& FGeoTime::operator-=(double DeltaSeconds)
{
	FGeoTime Tmp = (*this) - DeltaSeconds;
	TimeSeconds = Tmp.TimeSeconds;
	TimePartialSeconds = Tmp.TimePartialSeconds;
	return *this;
}

FGeoTime& FGeoTime::operator+=(const FGeoTime& Other)
{
	TimeSeconds += Other.TimeSeconds;
	TimePartialSeconds += Other.TimePartialSeconds;
	// Normalize partial seconds into [0,1)
	while (TimePartialSeconds >= 1.0)
	{
		TimePartialSeconds -= 1.0;
		++TimeSeconds;
	}
	while (TimePartialSeconds < 0.0)
	{
		TimePartialSeconds += 1.0;
		--TimeSeconds;
	}
	return *this;
}

FGeoTime& FGeoTime::operator-=(const FGeoTime& Other)
{
	// Reuse operator+= by adding the negated value
	return (*this) += FGeoTime(-Other.TimeSeconds, -Other.TimePartialSeconds);
}

// float overloads forward to double versions
FGeoTime& FGeoTime::operator+=(float DeltaSeconds)
{
	return (*this) += static_cast<double>(DeltaSeconds);
}

FGeoTime& FGeoTime::operator-=(float DeltaSeconds)
{
	return (*this) -= static_cast<double>(DeltaSeconds);
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
	return FString::Printf(TEXT("Client CurrentInputStep: Move(%.3f, %.3f) Dt=%.4f Time=%d+%.6f"), MovementInput.X,
		MovementInput.Y, DeltaTimeSeconds, Time.TimeSeconds, Time.TimePartialSeconds);
}

double FInputStep::GetTimeDiff(const FInputStep& Other) const
{
	return Time.GetTimeDiff(Other.Time);
}

bool FInputStep::IsEmpty() const
{
	const bool bMovementZero = MovementInput.IsNearlyZero(KINDA_SMALL_NUMBER);
	const bool bDeltaTimeSecondsZero = FMath::IsNearlyZero(DeltaTimeSeconds);
	const bool bTimeSecondsZero = Time.TimeSeconds == 0;
	const bool bTimePartialZero = FMath::IsNearlyZero(Time.TimePartialSeconds);
	return bMovementZero && bDeltaTimeSecondsZero && bTimeSecondsZero && bTimePartialZero;
}

void FInputStep::Empty()
{
	MovementInput = FVector2D::ZeroVector;
	DeltaTimeSeconds = 0.f;
	Time.TimeSeconds = 0;
	Time.TimePartialSeconds = 0.0;
}
