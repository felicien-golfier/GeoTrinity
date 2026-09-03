// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "World/GeoCurveCameraShake.h"

#include "Curves/CurveVector.h"

// Perlin noise crosses zero on every integer, so reading the three channels a whole number of steps apart keeps them
// from moving as one.
constexpr float AxisNoiseSeparation = 13.f;

void UGeoCurveCameraShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(GetDuration());
}

void UGeoCurveCameraShakePattern::StartShakePatternImpl(FCameraShakePatternStartParams const& /*Params*/)
{
	ensureMsgf(ShakeCurve, TEXT("%hs: ShakeCurve is not set on %s"), __FUNCTION__, *GetPathName());
	ElapsedTime = 0.f;
}

void UGeoCurveCameraShakePattern::UpdateShakePatternImpl(FCameraShakePatternUpdateParams const& Params,
														 FCameraShakePatternUpdateResult& OutResult)
{
	ElapsedTime += Params.DeltaTime;
	if (!ShakeCurve)
	{
		return;
	}

	FVector const Shake = ShakeCurve->GetVectorValue(ElapsedTime);
	float const Phase = ElapsedTime * Shake.Y;
	OutResult.Location = FVector(0.f, Shake.X * FMath::PerlinNoise1D(Phase),
								 Shake.X * FMath::PerlinNoise1D(Phase + AxisNoiseSeparation));
	OutResult.Rotation = FRotator(0.f, 0.f, Shake.Z * FMath::PerlinNoise1D(Phase + 2.f * AxisNoiseSeparation));
}

bool UGeoCurveCameraShakePattern::IsFinishedImpl() const
{
	return ElapsedTime >= GetDuration();
}

float UGeoCurveCameraShakePattern::GetDuration() const
{
	if (!ShakeCurve)
	{
		return 0.f;
	}

	float MinTime = 0.f;
	float MaxTime = 0.f;
	ShakeCurve->GetTimeRange(MinTime, MaxTime);
	return MaxTime;
}
