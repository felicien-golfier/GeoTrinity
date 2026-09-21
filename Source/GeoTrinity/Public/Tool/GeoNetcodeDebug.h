// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
struct FGeoPose;
struct FHitResult;

/**
 * Geo.DebugNetcode: draws and visual-logs lag compensation. Every function does nothing while the CVar is off, so call
 * sites stay one line.
 */
struct GEOTRINITY_API FGeoNetcodeDebug
{
	/** White arrow along the pose's yaw, kept Duration: the server's pose history trail. */
	static void DrawRecordedPose(AActor const* Actor, FGeoPose const& Pose, float Duration);

	/** Magenta arrow at the rewound pose, linked to the current one. */
	static void DrawRewoundPose(AActor const* Actor, FGeoPose const& RewoundPose, FGeoPose const& CurrentPose);

	/** Yellow segment the target is interpolated along this judge, with its timings in the visual log. */
	static void DrawHazardJudge(AActor const* Target, FVector2D LastLocation, float LastTime, FVector2D Location,
								float SpentTime, int32 NextSampleIndex, int32 SampleCount);

	/** Point where a sample tested the target: green inside the hazard, red outside. */
	static void DrawHazardSample(AActor const* Target, FVector2D Location, float SpentTime, int32 SampleIndex,
								 bool bInside);

	/** Cyan circle and heading of a bullet where it now flies. */
	static void DrawBullet(UObject const* Owner, FVector2D Location, FVector2D Direction, float Radius, float SpentTime);

	/** Red circle where a bullet was when it hit, linked to where the target stood then. */
	static void DrawBulletHit(AActor const* Target, FVector2D BulletLocation, float Radius, FVector2D TargetLocation,
							  float SpentTime);

	/** Orange segment of a bullet's wall sweep, and a circle where it stopped when bHitWall. */
	static void DrawWallSweep(UObject const* Owner, FVector const& Start, FVector const& End, float Radius,
							  bool bHitWall, FHitResult const& Hit);
};
