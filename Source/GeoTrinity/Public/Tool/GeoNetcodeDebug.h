// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class APawn;
struct FGeoPose;
struct FHitResult;

/**
 * Geo.DebugNetcode (bullets: Geo.DebugNetcode.Projectiles): draws and visual-logs lag compensation. Every function
 * does nothing while its CVar is off, so call sites stay one line. One colour code throughout: cyan is where something is at the current time, magenta where a
 * lagging view puts it in the past; a cyan rewind counterpart is drawn +15 Y so it never hides the magenta one.
 */
struct GEOTRINITY_API FGeoNetcodeDebug
{
	/** White arrow along the pose's yaw, kept Duration: the server's pose history trail. */
	static void DrawRecordedPose(AActor const* Actor, FGeoPose const& Pose, float Duration);

	/**
	 * Magenta point trail where the server has a remote player's pawn, linked to a cyan circle and arrow trail where
	 * its own client has it now: the gap is its lag. Drawn in the foreground so the pawn's mesh never hides them.
	 * Single-process PIE only, the one setup where the server can see a client world.
	 */
	static void DrawClientPose(APawn const* ServerPawn, float Duration);

	/** Magenta trail of rewound poses beside a cyan trail of current ones. */
	static void DrawRewoundPose(AActor const* Actor, FGeoPose const& RewoundPose, FGeoPose const& CurrentPose);

	/** Yellow segment the target is interpolated along this judge, with its timings in the visual log. */
	static void DrawHazardJudge(AActor const* Target, FVector2D LastLocation, float LastTime, FVector2D Location,
								float SpentTime, int32 NextSampleIndex, float Duration);

	/** Point where a sample tested the target: green inside the hazard, red outside. */
	static void DrawHazardSample(AActor const* Target, FVector2D Location, float SpentTime, int32 SampleIndex,
								 bool bInside);

	/** Cyan circle and heading of a bullet where it now flies. */
	static void DrawBullet(UObject const* Owner, FVector2D Location, FVector2D Direction, float Radius, float SpentTime);

	/**
	 * Magenta circle where a bullet was when it hit, linked to where the target stood then, beside a cyan circle where
	 * the bullet is now.
	 */
	static void DrawBulletHit(AActor const* Target, FVector2D BulletLocation, FVector2D CurrentBulletLocation,
							  float Radius, FVector2D TargetLocation, float SpentTime);

	/** Orange segment of a bullet's wall sweep, and a circle where it stopped when bHitWall. */
	static void DrawWallSweep(UObject const* Owner, FVector const& Start, FVector const& End, float Radius,
							  bool bHitWall, FHitResult const& Hit);

private:
	/** bEnabled (its CVar) and, in PIE, WorldContext is in the host's world: only the host's window shows debug. */
	static bool ShouldDraw(UObject const* WorldContext, bool bEnabled);

	/** ServerPawn's counterpart in the PIE client world that controls it, or null outside single-process PIE. */
	static APawn const* FindClientPawn(APawn const* ServerPawn);
};
