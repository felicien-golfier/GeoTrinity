// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Tutorial/GeoCatFaceMotion.h"

constexpr double EyeCycleSeconds = 5.4;
constexpr double EarCycleSeconds = 5.5;
constexpr double EarOffsetSeconds = .3;
constexpr double MouthHalfCycleSeconds = .38;
constexpr float MouthClosedScale = .35f;

struct FGeoCatFaceKey
{
	float Fraction;
	float Value;
};

// ---------------------------------------------------------------------------------------------------------------------
/** Linear value at Fraction of the cycle between consecutive keys, which run from 0 to 1. */
static float Evaluate(TArrayView<FGeoCatFaceKey const> const Keys, double const Fraction)
{
	int32 Next = 1;
	while (Next < Keys.Num() - 1 && Fraction > Keys[Next].Fraction)
	{
		++Next;
	}

	FGeoCatFaceKey const& From = Keys[Next - 1];
	FGeoCatFaceKey const& To = Keys[Next];
	return FMath::Lerp(From.Value, To.Value,
					   FMath::Clamp((Fraction - From.Fraction) / (To.Fraction - From.Fraction), 0., 1.));
}

// ---------------------------------------------------------------------------------------------------------------------
float GeoCatFaceMotion::GetEyeScale(double const Seconds)
{
	static FGeoCatFaceKey const Keys[] = {{0.f, 1.f}, {.4f, 1.f},	{.44f, .55f}, {.56f, .55f}, {.6f, 1.f},
										  {.9f, 1.f}, {.94f, .08f}, {.97f, 1.f},  {1.f, 1.f}};
	return Evaluate(Keys, FMath::Fmod(Seconds, EyeCycleSeconds) / EyeCycleSeconds);
}

// ---------------------------------------------------------------------------------------------------------------------
float GeoCatFaceMotion::GetEarAngle(double const Seconds, int32 const EarIndex)
{
	static FGeoCatFaceKey const Keys[] = {{0.f, 0.f}, {.86f, 0.f}, {.9f, -5.f}, {.94f, 2.f}, {1.f, 0.f}};
	double const Cycle = FMath::Fmod(Seconds - EarIndex * EarOffsetSeconds + EarCycleSeconds, EarCycleSeconds);
	return Evaluate(Keys, Cycle / EarCycleSeconds);
}

// ---------------------------------------------------------------------------------------------------------------------
float GeoCatFaceMotion::GetMouthScale(double const Seconds)
{
	double const Fraction = FMath::Fmod(Seconds, 2. * MouthHalfCycleSeconds) / MouthHalfCycleSeconds;
	return FMath::Lerp(MouthClosedScale, 1.f, static_cast<float>(Fraction < 1. ? Fraction : 2. - Fraction));
}
